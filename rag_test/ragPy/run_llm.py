'''
SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
SPDX-License-Identifier: GPL-3.0-or-later
'''

import logging
import torch
from tqdm import tqdm
from typing import List
import json

from concurrent.futures import ProcessPoolExecutor, ThreadPoolExecutor, as_completed

from docProcess import embedding_texts, index_search, split_documents, gen_index
from prompt_template import get_prompt

from transformers import AutoModelForCausalLM, AutoTokenizer, LlamaForCausalLM, LlamaTokenizer

from langchain.llms import HuggingFacePipeline

from transformers import (
    GenerationConfig,
    pipeline,
)

MODELS_PATH = "./models"
MAX_NEW_TOKENS = 4096

INDEX_DIRECTORY = 'indexes/indexes_min_10_L2/'

def load_full_model(model_path, device_type):
    if device_type.lower() in ["mps", "cpu"]:
        logging.info("Using LlamaTokenizer")
        tokenizer = LlamaTokenizer.from_pretrained(model_path)
        model = LlamaForCausalLM.from_pretrained(model_path)
    else:
        logging.info("Using AutoModelForCausalLM for full models")
        tokenizer = AutoTokenizer.from_pretrained(model_path)
        model = AutoModelForCausalLM.from_pretrained(
            model_path,
            device_map="auto",
            torch_dtype=torch.float16,
            low_cpu_mem_usage=True,
            cache_dir=MODELS_PATH,
            trust_remote_code=True,  # set these if you are using NVIDIA GPU
            load_in_4bit=True,
            bnb_4bit_quant_type="nf4",
            bnb_4bit_compute_dtype=torch.float16,
            max_memory={0: "15GB"},  # Uncomment this line with you encounter CUDA out of memory errors
        )
        model.tie_weights()
    
    return model, tokenizer

def load_model(device_type, model_path):
    logging.info(f"Loading Model: {model_path}, on: {device_type}")

    model, tokenizer = load_full_model(model_path, device_type)
    generation_config = GenerationConfig.from_pretrained(model_path)

    pipe = pipeline(
        "text-generation",
        model=model,
        tokenizer=tokenizer,
        max_length=MAX_NEW_TOKENS,
        temperature=0.2,
        # top_p=0.95,
        repetition_penalty=1.15,
        generation_config=generation_config,
    )

    local_llm = HuggingFacePipeline(pipeline=pipe)
    logging.info("Local LLM Loaded")

    return local_llm

def calculate_intersection(str1, str2):
    # 将两个字符串转换成集合
    set1 = set(str1)
    set2 = set(str2)

    intersection = set1 & set2
    result = ''.join(intersection)
    
    return intersection

def process_question(prompt, answer, llm):
    response = llm(prompt)

    common = list(calculate_intersection(response, answer))
    answer_list = list(answer)
    acc = len(common) / len(answer_list)

    return acc

def process_min_v(min_v, question_vector):
    embedding_model_name = 'BAAI_bge-large-zh-v1.5'
    device_type="cuda"

    print(f"process {min_v}.......")

    llm = load_model(device_type=device_type, model_path='../../models/chinese-alpaca-2-7b-hf/')
    chunks = split_documents("./SOURCE_DOCUMENTS/", min_text_len=min_v, save_chunk=False, chunk_strategy='min')

    from utils import load_qa
    _qa = load_qa()
    question = _qa['question']
    answer = _qa['answer']
        
    accurate = dict()
    # for s in ['flat', 'ivf_flat', 'pq', 'ivf_pq']:
    #     index = gen_index(chunks, embedding_model_name, device_type, index_type=s, save_index=False, chunk_strategy='min')
    #     print(f"process {s} index.........")        

    #     prompt = get_prompt(question[i], search_result)

    #     acc = 0
    #     with ThreadPoolExecutor(len(question)) as exe:
    #         for i in tqdm(range(len(question))):
    #             k = 10
    #             D, I = index.search(question_vector[i], k)
    #             search_result = [chunks[I[0][i]] for i in range(len(I[0]))]
    #             prompt = get_prompt(question[i], search_result)

    #             future = exe.submit(process_question, prompt, answer[i], llm)
    #             acc += future.result()

    #         accurate[s] = acc / len(question)
    print("save result.....")
    # result[min_v] = accurate
    return accurate

def min_v_acc(min_value, max_value, step_value, result_file_path, device_type):
    embedding_model_name = 'BAAI_bge-large-zh-v1.5'

    llm = load_model(device_type=device_type, model_path='../../models/chinese-alpaca-2-7b-hf/')

    from utils import load_qa
    _qa = load_qa()
    question = _qa['question']
    answer = _qa['answer']

    question_vector = []
    for i in range(len(question)):
        embedding_vector, _, _ = embedding_texts([question[i]], embedding_model_name, device_type)
        question_vector.append(embedding_vector)

    print(len(question_vector))
    result = dict()
    #result_file_path = './min_v_acc.json'    
    for min_v in range(min_value, max_value, step_value):
        print('*'*50)
        print(min_v)
        print('*'*50)
        chunks = split_documents("./SOURCE_DOCUMENTS/", min_text_len=min_v, save_chunk=False, chunk_strategy='min')
        
        accurate = dict()
        for s in ['flat', 'ivf_flat', 'pq', 'ivf_pq']:
            index = gen_index(chunks, embedding_model_name, device_type, index_type=s, save_index=False, chunk_strategy='min')
            print(f"process {s} index.........")
            acc = 0

            for i in tqdm(range(len(question))):
                k = 10
                D, I = index.search(question_vector[i], k)
                search_result = [chunks[I[0][i]] for i in range(len(I[0]))]

                prompt = get_prompt(question[i], search_result)
                response = llm(prompt)

                common = list(calculate_intersection(response, answer[i]))
                answer_list = list(answer[i])
                acc += len(common) / len(answer_list)
            accurate[s] = acc / len(question)
        print("save result.....")
        result[min_v] = accurate        

        with open(result_file_path, 'w') as file:
            json.dump(result, file, indent=4)

def run_qa():
    embedding_model_name = 'BAAI_bge-large-zh-v1.5'
    device_type="cuda"

    llm = load_model(device_type=device_type, model_path='../../models/chinese-alpaca-2-7b-hf/')

    while True:
        query = input("\nEnter a query: ")
        if query == "exit":
            break
        embedding_vector, _, _ = embedding_texts([query], embedding_model_name, device_type)
        search_result = index_search(embedding_vector, INDEX_DIRECTORY, device_type = device_type, use_file=True, index_type='flat', chunk_strategy='min')

        prompt = get_prompt(query, search_result)
        response = llm(prompt)

        print('-'*50)
        print(f"Q: {query}\nA: {response}")

def main():
    #min_v_acc(10, 150, 5, './min_v_acc_0.json', device_type='cuda:1')
    run_qa()

    # embedding_model_name = 'BAAI_bge-large-zh-v1.5'
    # device_type="cuda"

    # llm = load_model(device_type=device_type, model_path='../../models/chinese-alpaca-2-7b-hf/')

    # from utils import load_qa
    # _qa = load_qa()
    # question = _qa['question']
    # answer = _qa['answer']

    # accurate = []
    
    # for s in ['flat', 'ivf_flat', 'pq', 'ivf_pq']:
    #     acc = 0
    #     for i in tqdm(range(len(question))):
    #         embedding_vector, _, _ = embedding_texts([question[i]], embedding_model_name, device_type)
    #         search_result = index_search(embedding_vector, INDEX_DIRECTORY, index_type=s, chunk_strategy='min')

    #         prompt = get_prompt(question[i], search_result)
    #         response = llm(prompt)

    #         #print(f"Q: {question[i]}\nA: {response}")
    #         common = list(calculate_intersection(response, answer[i]))
    #         answer_list = list(answer[i])
    #         acc += len(common) / len(answer_list)
    #     #print(f"Accuracy: {acc / len(question)}")
    #     _acc = {s+'_min': acc / len(question)}
    #     accurate.append(_acc)
    
    # print('*'*50)
    # print(f"result: {accurate}")
    # print('*'*50)    

    
        

if __name__ == "__main__":
    logging.basicConfig(
        format="%(asctime)s - %(levelname)s - %(filename)s:%(lineno)s - %(message)s", level=logging.INFO
    )
    main()

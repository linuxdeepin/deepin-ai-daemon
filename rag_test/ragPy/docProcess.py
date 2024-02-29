'''
SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
SPDX-License-Identifier: GPL-3.0-or-later
'''

import json
import os
import logging
import re
import numpy as np
import faiss

from loadDocuments import load_documents

from typing import List
from langchain.embeddings import HuggingFaceBgeEmbeddings

MINTXTLEN = 10
MAXTXTLEN = 450

MINCHUNKSTRATEGE = 'min'
MAXCHUNKSTRATEGE = 'max'
#DEFUALTCHUNKSTRATEGE = 'avg'

INDEX_DIRECTORY = 'indexes/indexes_min_10_L2/'
CHUNKS_DIRECTORY = 'indexes/indexes_min_10_L2/'

INDEX_TYPE = ['flat', 'ivf_flat', 'pq', 'ivf_pq']

def single_sentence_split(text: str) -> List[str]:
    if len(text) <= MAXTXTLEN:
        return [text]
    sentence_split = []
    
    sentence_split += single_sentence_split(text[0:round(len(text)/2)])
    sentence_split += single_sentence_split(text[round(len(text)/2):len(text)])

    return sentence_split

def split_text_with_regex(
    text: str, separator: str, keep_separator: bool = True
) -> List[str]:
    # Now that we have the separator, split the text
    if separator:
        if keep_separator:
            # The parentheses in the pattern keep the delimiters in the result.
            _splits = re.split(f"({separator})", text)
            splits = [_splits[i + 1] for i in range(1, len(_splits), 2)]
            if len(_splits) % 2 == 0:
                splits += _splits[-1:]
            splits = [_splits[0]] + splits
        else:
            splits = re.split(f"({separator})", text)
    else:
        splits = list(text)

    return [s for s in splits if s != ""]

def split_text(text, min_text_len=10, chunk_size=1, chunk_overlap=0, chunk_strategy=MAXCHUNKSTRATEGE):
    separator = ["\n", "。"]
    split_text = split_text_with_regex(text, separator)

    for i in range(0, len(split_text)):
        if len(split_text[i]) > MAXTXTLEN:
            sentence_split = single_sentence_split(split_text[i])
            split_text.pop(i)
            for j in range(0, len(sentence_split)):
                split_text.insert(i, sentence_split[j])
                i += 1
    
    chunks = []
    chunk = ''
    chunk_size = 0
    if chunk_strategy == MINCHUNKSTRATEGE:
        for i in range(0, len(split_text)):
            chunk_size += len(split_text[i])
            if chunk_size > min_text_len:
                if chunk_size > MAXTXTLEN:
                    chunks.append(chunk)
                    chunk = split_text[i]
                    chunk_size = 0
                else:
                    chunk += split_text[i]
                    chunks.append(chunk)
                    chunk = ''
                    chunk_size = 0
            else:            
                chunk += split_text[i]

    elif chunk_strategy == MAXCHUNKSTRATEGE:
        for i in range(0, len(split_text)):
            chunk_size += len(split_text[i])
            if chunk_size > MAXTXTLEN:
                chunks.append(chunk)
                chunk = split_text[i]
                chunk_size = 0
            else:            
                chunk += split_text[i]
    else:
        for i in range(0, len(split_text)):
            chunks.append(split_text[i])

    print(f'sentence_num:{len(split_text)}       chunks_num:{len(chunks)}')
    
    return chunks

def embedding_texts(texts, model_name, device_type):
    embeddings = HuggingFaceBgeEmbeddings(
        model_name=model_name,
        cache_folder='./sentence-transformers',
        model_kwargs={"device": device_type},
        encode_kwargs={'normalize_embeddings': True},
    )
    
    embedding_vector = np.array(embeddings.embed_documents(texts)).astype(np.float32)

    vector_sum = embedding_vector.shape[0]
    embedding_dim = embedding_vector.shape[1]

    return embedding_vector, vector_sum, embedding_dim

def split_documents(documents, min_text_len=10, save_chunk=False, chunk_strategy=MAXCHUNKSTRATEGE):    
    docs = load_documents(documents)
    texts = docs[0].page_content    

    chunks = split_text(texts, min_text_len=min_text_len, chunk_strategy=chunk_strategy)

    if save_chunk:
        with open(CHUNKS_DIRECTORY + chunk_strategy + '_chunks.json', 'w') as file:
            json.dump(chunks, file, ensure_ascii=False)

    return chunks

def split_documents_test(documents, min_text_len=10, save_chunk=False, chunk_strategy=MAXCHUNKSTRATEGE):
    docs = load_documents(documents)

    for doc in docs:
        text = doc.page_content
        id = doc.metadata['source'].split('source_context/')[-1].split('.')[0]
        
        chunk = split_text(text, min_text_len=min_text_len, chunk_strategy=chunk_strategy)
        print(len(chunk))

    #chunks = split_text(texts, min_text_len=min_text_len, chunk_strategy=chunk_strategy)

    # if save_chunk:
    #     with open(CHUNKS_DIRECTORY + chunk_strategy + '_chunks.json', 'w') as file:
    #         json.dump(chunks, file, ensure_ascii=False)

    # return chunks

def gen_index(chunks, model_name, device_type, index_type, ivf_nlists=256, save_index=False, chunk_strategy=MAXCHUNKSTRATEGE):    
    embedding_vector, vector_sum, embedding_dim = embedding_texts(chunks, model_name, device_type)
    print(f"index generate on {device_type}...")
    print(f"vector_sum: {vector_sum}  embedding_dim: {embedding_dim}")
    print(f"index type: {index_type}")

    if index_type == 'flat':
        index = faiss.IndexFlatL2(embedding_dim)
        index.add(embedding_vector)
    
    elif index_type == 'ivf_flat':
        index = faiss.IndexIVFFlat(
            faiss.IndexFlatL2(embedding_dim),
            embedding_dim,
            100,
            faiss.METRIC_L2)
        index.train(embedding_vector)
        index.add(embedding_vector)
    
    elif index_type == 'pq':
        index = faiss.IndexPQ(embedding_dim, 256, 8)
        index.train(embedding_vector)
        index.add(embedding_vector)
    
    elif index_type == 'ivf_pq':
        index = faiss.IndexIVFPQ(
            faiss.IndexFlatL2(embedding_dim), 
            embedding_dim, 
            ivf_nlists,
            256, 8)
        index.train(embedding_vector)
        index.add(embedding_vector)

    if save_index:
        # save index
        faiss.write_index(index, INDEX_DIRECTORY + chunk_strategy + '_' + index_type + '_index.faiss')
    
    return index

def index_search(xq, index_path, index_type, device_type, chunk_strategy=MAXCHUNKSTRATEGE, use_file=True):
    if use_file:
        index_file = index_path + chunk_strategy + '_' + index_type + '_index.faiss'
        index = faiss.read_index(index_file)
        k = 10
        D, I = index.search(xq, k)
    
    else:
        embedding_model_name = 'BAAI_bge-large-zh-v1.5'
        device_type="cuda"

        chunks = split_documents("./SOURCE_DOCUMENTS/", save_chunk=False, chunk_strategy=MINCHUNKSTRATEGE)
        index = gen_index(chunks, embedding_model_name, device_type, index_type=index_type, save_index=False, chunk_strategy=MINCHUNKSTRATEGE)
        print('*'*10)
        print(index)


    with open(CHUNKS_DIRECTORY + chunk_strategy + '_chunks.json', 'r') as file:
        chunks = json.load(file)

    # for i in range(len(I[0])):
    #     print(chunks[I[0][i]])
    
    return [chunks[I[0][i]] for i in range(len(I[0]))]
    
def main():
    # embedding_model_name = 'BAAI_bge-large-zh-v1.5'
    # device_type="cuda"

    chunks = split_documents_test("./source_context/", min_text_len=20, save_chunk=False, chunk_strategy=MINCHUNKSTRATEGE)
    # for s in INDEX_TYPE:
    #     index = gen_index(chunks, embedding_model_name, device_type, index_type=s, save_index=True, chunk_strategy=MINCHUNKSTRATEGE)
    #gen_index(chunks, embedding_model_name, device_type, index_type='flat', save_index=True)        


if __name__ == "__main__":
    logging.basicConfig(
        format="%(asctime)s - %(levelname)s - %(filename)s:%(lineno)s - %(message)s", level=logging.INFO
    )
    main()


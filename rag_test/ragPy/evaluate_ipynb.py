'''
SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
SPDX-License-Identifier: GPL-3.0-or-later
'''
# %%
from beir import util, LoggingHandler
from beir.retrieval import models
from beir.datasets.data_loader import GenericDataLoader
from beir.retrieval.evaluation import EvaluateRetrieval
from beir.retrieval.search.dense import DenseRetrievalExactSearch as DRES

from beir.retrieval.search.dense import PQFaissSearch, HNSWFaissSearch, FlatIPFaissSearch, HNSWSQFaissSearch  

from langchain.embeddings import HuggingFaceBgeEmbeddings
import faiss
import logging
import pathlib, os

#### Just some code to print debug information to stdout
logging.basicConfig(format='%(asctime)s - %(message)s',
                    datefmt='%Y-%m-%d %H:%M:%S',
                    level=logging.INFO,
                    handlers=[LoggingHandler()])
#### /print debug information to stdout

#### Download scifact.zip dataset and unzip the dataset
# dataset = "scifact"
# url = "https://public.ukp.informatik.tu-darmstadt.de/thakur/BEIR/datasets/{}.zip".format(dataset)
# out_dir = os.path.join(pathlib.Path(__file__).parent.absolute(), "datasets")
# data_path = util.download_and_unzip(url, out_dir)

# %%
data_path = './datasets/scifact/'

#### Provide the data_path where scifact has been downloaded and unzipped
corpus, queries, qrels = GenericDataLoader(data_folder=data_path).load(split="test")

# embedding_model_name = 'BAAI_bge-large-zh-v1.5'
# device_type="cuda"
# model = HuggingFaceBgeEmbeddings(
#         model_name=embedding_model_name,
#         cache_folder='./sentence-transformers',
#         model_kwargs={"device": device_type},
#         encode_kwargs={'normalize_embeddings': True},
#     )

model_path = "./sentence-transformers/msmarco-distilbert-base-tas-b"
model = models.SentenceBERT(model_path)

faiss_search = FlatIPFaissSearch(model, batch_size=128)
# faiss_search.load(input_dir=data_path, prefix='train', ext='1')

retriever = EvaluateRetrieval(faiss_search, score_function="dot") # or "cos_sim"
results = retriever.retrieve(corpus, queries)
print(results)

# %%
import numpy as np
from beir.retrieval.search.dense.faiss_index import FaissIndex

embedding_model_name = 'BAAI_bge-large-zh-v1.5'
device_type="cuda"
embeddings = HuggingFaceBgeEmbeddings(
        model_name=embedding_model_name,
        cache_folder='./sentence-transformers',
        model_kwargs={"device": device_type},
        encode_kwargs={'normalize_embeddings': True},
    )

texts = ['aaa', 'bbb', 'ccc']
embedding_vector = np.array(embeddings.embed_documents(texts)).astype(np.float32)

contexts = ['haello world', 'hello world', 'hello world', 'hello world', 'aaa', 'hello worldaaa', 'hello world', 'heaaallo world', 'hello world', 'hello world']
ids = ['10043570', '105743701', '10034582', '100876583', '109004', '10097805', '10069876', '105423607', '10687008', '10056469']
ids = np.array(ids, dtype=np.int64)
embedding_contexts = np.array(embeddings.embed_documents(contexts)).astype(np.float32)
index = faiss.IndexIDMap(faiss.IndexFlatL2(embedding_contexts.shape[1]))
index.add_with_ids(embedding_contexts, ids)
#index = faiss.read_index('./indexes/indexes_min_10_L2/min_flat_index.faiss')
k = 10
print(embedding_vector.shape)
D, I = index.search(embedding_vector, k)

print(D)

# %%
from opencc import OpenCC
import json
import jsonlines
import tqdm

miracl_dir = './miracl/'
miracl_corpus_dir = './tmp/'

def traditional_to_simplified(traditional_text):
    cc = OpenCC('t2s')  # 创建一个繁体到简体的转换器
    simplified_text = cc.convert(traditional_text)  # 进行转换
    return simplified_text

def t2s_id(id):
    results = []
    num = 0
    with open(miracl_corpus_dir + 'docs-' + str(id) + '.jsonl', 'r') as file:
        for line in file:
            print(f"process: {id}  num: {num}")
            num += 1
            
            data = json.loads(line)
            result = dict()
            result['docid'] = data['docid']
            result['title'] = traditional_to_simplified(data['title'])
            result['text'] = traditional_to_simplified(data['text'])
            results.append(result)

    with jsonlines.open(miracl_corpus_dir + 'docs-t2s-' + str(id) + '.jsonl', mode='w') as writer:
        for obj in results:
            writer.write(obj)

def t2s():
    results = []
    num = 1
    for i in range(1, 10):
        with open('docs-' + str(i) + '.jsonl', 'r') as file:
            for line in file:
                print(num)
                num += 1
                data = json.loads(line)
                result = dict()
                result['docid'] = data['docid']
                result['title'] = traditional_to_simplified(data['title'])
                result['text'] = traditional_to_simplified(data['text'])
                results.append(result)

        with jsonlines.open('docs-t2s-' + str(i) + '.jsonl', mode='w') as writer:
            for obj in results:
                writer.write(obj)

# # 要转换的繁体中文文本
# traditional_text = ""

# # 调用函数进行转换
# simplified_text = traditional_to_simplified(traditional_text)

# print("繁体中文文本：", traditional_text)
# print("简体中文文本：", simplified_text)

# %%
import csv

miracl_corpus_dir = '../datasets/miracl-corpus/'

reader = csv.reader(open('../datasets/miracl/qrels/qrels_200.tsv', encoding="utf-8"), 
                            delimiter="\t", quoting=csv.QUOTE_MINIMAL)

reader_topics = csv.reader(open('../datasets/miracl/topics/topics.miracl-v1.0-zh-train.tsv', encoding="utf-8"), 
                            delimiter="\t", quoting=csv.QUOTE_MINIMAL)

query_ids, corpus_ids, relevants= [], [], []
for id, row in enumerate(reader):
    query_id, _, corpus_id, relevant = row[0], row[1], row[2], int(row[3])
    query_ids.append(query_id)
    corpus_ids.append(corpus_id)
    relevants.append(relevant)

questions = []
for id, row in enumerate(reader_topics):
    query_id, question = row[0], row[1]
    if query_id in query_ids:
        questions.append(row)

with open('../datasets/miracl/topics/topics_200.csv', 'w', newline='') as f:
    writer = csv.writer(f)
    writer.writerows(questions)

# %% [markdown]
# ##整体流程

# %%
import pytrec_eval
import json
import jsonlines
import csv

miracl_qrels_dir = '../datasets/miracl/qrels/qrels_200.tsv'
miracl_corpus_dir = '../datasets/miracl/docs_200.jsonl'
miracl_topics_dir = '../datasets/miracl/topics/topics_200.csv'

embedding_model_name = 'BAAI_bge-large-zh-v1.5'
device_type="cuda"
embeddings = HuggingFaceBgeEmbeddings(
        model_name=embedding_model_name,
        cache_folder='./sentence-transformers',
        model_kwargs={"device": device_type},
        encode_kwargs={'normalize_embeddings': True},
    )

texts, text_ids = [], []
with jsonlines.open(miracl_corpus_dir, mode='r') as file:
    for data in file:
        texts.append(data['text'])
        text_ids.append(data['docid'].replace('#', ''))

query_ids, questions = [], []
topics_reader = csv.reader(open(miracl_topics_dir, encoding="utf-8"), 
                            delimiter="\t", quoting=csv.QUOTE_MINIMAL)
for id, row in enumerate(topics_reader):
    id = row[0].split(',')[0].replace('#', '')
    q = row[0].split(',')[1]
    query_ids.append(id)
    questions.append(q)

qrels_query_ids, qrels_doc_ids = [], []
qrels_reader = csv.reader(open(miracl_qrels_dir, encoding="utf-8"), 
                            delimiter="\t", quoting=csv.QUOTE_MINIMAL)
for id, row in enumerate(qrels_reader):
    query_id, doc_id, score = row[0].replace('#', ''), row[2].replace('#', ''), int(row[3])
    if query_id not in qrels_query_ids:
        qrels_query_ids.append(query_id)
        bool_tmp = True
        docs = dict()
        qrels_doc_ids.append(docs)
        
    docs[doc_id] = score

for idx, q_id in enumerate(qrels_query_ids):
    qrels[q_id] = qrels_doc_ids[idx]

embedding_vector = np.array(embeddings.embed_documents(texts)).astype(np.float32)

index = faiss.IndexIDMap(faiss.IndexFlatL2(embedding_vector.shape[1]))
index.add_with_ids(embedding_vector, np.array(text_ids, dtype=np.int64))

xq = np.array(embeddings.embed_documents(questions)).astype(np.float32)
k = 100
D, I = index.search(xq, k)
#欧式距离转化为相似性分数
#内积值转相似性分数： [ \text{cosine similarity} = \frac{A \cdot B}{|A| \cdot |B|} ]
similarity_scores = 1 / (1 + D)

result = []
for ids, scores in zip(I, similarity_scores):
    result_doc_scores = dict()
    for id, score in zip(ids, scores):
        result_doc_scores[str(id)] = float(score)
    result.append(result_doc_scores)

results = dict()
for idx, q_id in enumerate(query_ids):
    results[q_id] = result[idx]

k_values = [1, 2, 3, 4, 5]
map_string = "map_cut." + ",".join([str(k) for k in k_values])
ndcg_string = "ndcg_cut." + ",".join([str(k) for k in k_values])
recall_string = "recall." + ",".join([str(k) for k in k_values])
precision_string = "P." + ",".join([str(k) for k in k_values])
print(f"******{map_string}******")
evaluator = pytrec_eval.RelevanceEvaluator(qrels, {map_string, ndcg_string, recall_string, precision_string})
scores = evaluator.evaluate(results)

ndcg = {}
_map = {}
recall = {}
precision = {}
        
for k in k_values:
    ndcg[f"NDCG@{k}"] = 0.0
    _map[f"MAP@{k}"] = 0.0
    recall[f"Recall@{k}"] = 0.0
    precision[f"P@{k}"] = 0.0
    
for query_id in scores.keys():
    for k in k_values:
        ndcg[f"NDCG@{k}"] += scores[query_id]["ndcg_cut_" + str(k)]
        _map[f"MAP@{k}"] += scores[query_id]["map_cut_" + str(k)]
        recall[f"Recall@{k}"] += scores[query_id]["recall_" + str(k)]
        precision[f"P@{k}"] += scores[query_id]["P_"+ str(k)]
        
for k in k_values:
    ndcg[f"NDCG@{k}"] = round(ndcg[f"NDCG@{k}"]/len(scores), 5)
    _map[f"MAP@{k}"] = round(_map[f"MAP@{k}"]/len(scores), 5)
    recall[f"Recall@{k}"] = round(recall[f"Recall@{k}"]/len(scores), 5)
    precision[f"P@{k}"] = round(precision[f"P@{k}"]/len(scores), 5)
        
for eval in [ndcg, _map, recall, precision]:
    for k in eval.keys():
        print("{}: {}".format(k, eval[k]))


# %%



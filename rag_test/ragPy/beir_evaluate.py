'''
SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
SPDX-License-Identifier: GPL-3.0-or-later
'''

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

data_path = './datasets/scifact/'

#### Provide the data_path where scifact has been downloaded and unzipped
corpus, queries, qrels = GenericDataLoader(data_folder=data_path).load(split="train")

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

retriever = EvaluateRetrieval(faiss_search, score_function="cos_sim") # or "cos_sim"
results = retriever.retrieve(corpus, queries)

print(f"********{results['2']}")

logging.info("Retriever evaluation for k in: {}".format(retriever.k_values))
ndcg, _map, recall, precision = retriever.evaluate(qrels, results, retriever.k_values)

mrr = retriever.evaluate_custom(qrels, results, retriever.k_values, metric="mrr")
recall_cap = retriever.evaluate_custom(qrels, results, retriever.k_values, metric="r_cap")
hole = retriever.evaluate_custom(qrels, results, retriever.k_values, metric="hole")


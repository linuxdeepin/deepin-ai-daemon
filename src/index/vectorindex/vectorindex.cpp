// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vectorindex.h"
#include "../global_define.h"

#include <QList>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>

#include <faiss/IndexFlat.h>
#include <faiss/IndexIVFPQ.h>
#include <faiss/index_io.h>
#include <faiss/index_factory.h>
#include <faiss/utils/random.h>
#include <faiss/IndexIDMap.h>
#include <faiss/IndexShards.h>
#include <faiss/IndexFlatCodes.h>

VectorIndex::VectorIndex(QObject *parent)
{

}

void VectorIndex::init()
{

}

bool VectorIndex::createIndex(int d, const QVector<float> &embeddings, const QVector<faiss::idx_t> &ids, const QString &indexKey)
{
//    //-----index factory
//    //QString ivfPqIndexKey = "IVF256, PQ8";
//    //index.reset(faiss::index_factory(d, flatIndexKey.toStdString().c_str()));

    /* n:向量个数
     * 一条1024dim，f32的向量大小为4KB，1w条40MB，检索时间为3ms左右；
     * 创建索引时按照n的大小(索引文件大小、检索时间)选择IndexType
     */
    int n = embeddings.count() / d;
    if (n < 1000)
        //小于1000个向量，直接Flat
        return createFlatIndex(d, embeddings, ids, indexKey);
    else if (n < 1000000)
        return createIvfFlatIndex(d, embeddings, indexKey);

    return false;
}

bool VectorIndex::updateIndex(int d, const QVector<float> &embeddings, const QVector<faiss::idx_t> &ids, const QString &indexKey)
{
    if (embeddings.isEmpty())
        return false;

    //TODO:会加载索引、消耗资源,存储信息到DB
    int nTotal = getIndexNTotal(indexKey);

    if (nTotal < 1000)
        //小于1000个向量，直接Flat
        return updateFlatIndex(d, embeddings, ids, indexKey);
    else if (nTotal < 1000000) {
        return updateIvfFlatIndex(d, embeddings, indexKey);
    }
    return false;

    //faiss::Index *newIndex = faiss::index_factory(d, "Flat");
    //faiss::IndexFlat *newIndex = new faiss::IndexFlat(d);
    //faiss::IndexFlat *newIndex1 = new faiss::IndexFlat(d);
    //faiss::idx_t n = ids.count();
//    faiss::IndexIDMap indexMap(index);
//    indexMap.add_with_ids(n, embeddings.data(), ids.data());
    //newIndex->add(n, embeddings.data());
    //newIndex1->add(n, embeddings.data());

    //faiss::IndexIDMapTemplate<faiss::Index> *newIndex = dynamic_cast<faiss::IndexIDMapTemplate<faiss::Index>*>(indexMap.index);
    //faiss::IndexFlatCodes *newIndex = dynamic_cast<faiss::IndexFlatCodes*>(index);

    //合并
    //faiss::idx_t aa1 = oldIndex->ntotal;
    //faiss::idx_t aa2 = indexMap.ntotal;
    //newIndex->merge_from(*oldIndex);
   // qInfo() << newIndex->ntotal;
    //oldIndex->merge_from(*indexMap.index);
    //faiss::idx_t aa = oldIndex->ntotal;

}

bool VectorIndex::deleteIndex(const QString &indexKey, const QVector<faiss::idx_t> &deleteID)
{
    return deleteFlatIndex(deleteID, indexKey);
}

bool VectorIndex::saveIndexToFile(const faiss::Index *index, const QString &indexKey, const QString &indexType)
{
    qInfo() << "save faiss index...";
    QString indexDirStr = workerDir() + QDir::separator() + indexKey;
    QDir indexDir(indexDirStr);

    if (!indexDir.exists()) {
        if (!indexDir.mkpath(indexDirStr)) {
            qWarning() << indexKey << " directory isn't exists and can't create!";
            return false;
        }
    }
    QString indexPath = indexDir.path() + QDir::separator() + indexType + ".faiss";
    qInfo() << "index file save to " + indexPath;

    try {
        faiss::write_index(index, indexPath.toStdString().c_str());
        return true;
    } catch (faiss::FaissException &e) {
        std::cerr << "Faiss error: " << e.what() << std::endl;
    }

    return true;
}

faiss::Index* VectorIndex::loadIndexFromFile(const QString &indexKey, const QString &indexType)
{
    qInfo() << "load faiss index...";
    QString indexDirStr = workerDir() + QDir::separator() + indexKey;
    QDir indexDir(indexDirStr);

    if (!indexDir.exists()) {
        if (!indexDir.mkpath(indexDirStr)) {
            qWarning() << indexKey << " directory isn't exists!";
            return {};
        }
    }
    QString indexPath = indexDir.path() + QDir::separator() + indexType + ".faiss";
    qInfo() << "load index file from " + indexPath;

    faiss::Index *index;
    try {
        index = faiss::read_index(indexPath.toStdString().c_str());
    } catch (faiss::FaissException &e) {
        std::cerr << "Faiss error: " << e.what() << std::endl;
        return nullptr;
    }
    return index;
}

QVector<faiss::idx_t> VectorIndex::vectorSearch(int topK, const float *queryVector, const QString &indexKey)
{
    //读取索引文件
    faiss::Index *index = loadIndexFromFile(indexKey, kFaissFlatIndex);
    if (!index)
        return {};
    //向量检索
    QVector<float> D1(topK);
    QVector<faiss::idx_t> I1(topK);
    index->search(1, queryVector, topK, D1.data(), I1.data());

    //TODO:检索结果后处理-去重、过于相近或远
//    QVector<faiss::idx_t> nonDupIndex;
//    QMap<float, bool> seen;
//    removeDupIndex(index, topK, 0, nonDupIndex, queryVector, seen);

    return I1;
}

QStringList VectorIndex::loadTexts(const QVector<faiss::idx_t> &ids, const QString &indexKey)
{
    QStringList docFilePaths;
    QStringList texts;

    QString dataInfoPath = workerDir() + QDir::separator() + indexKey + kDataInfo;
    QFile dataInfoFile(dataInfoPath);
    if (!dataInfoFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Failed to open data info file.";
        return {};
    }

    QByteArray jsonData = dataInfoFile.readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);

    if (jsonDoc.isNull()) {
        qDebug() << "Failed to parse JSON file.";
        return {};
    }

    QJsonObject dataInfoJson = jsonDoc.object();
    for (faiss::idx_t id : ids) {
        QJsonObject contentObj = dataInfoJson.value(QString::number(id)).toObject();
        docFilePaths.append(contentObj["docFilePath"].toString());
        texts.append(contentObj["text"].toString());
    }
    return texts;
}

bool VectorIndex::createFlatIndex(int d, const QVector<float> &embeddings, const QVector<faiss::idx_t> &ids, const QString &indexKey)
{
    faiss::Index *index = faiss::index_factory(d, kFaissFlatIndex);
    faiss::idx_t n = embeddings.count() / d;

    faiss::IndexIDMap indexMap(index);
    indexMap.add_with_ids(n, embeddings.data(), ids.data());
    //index->add(n, embeddings.data());

    return saveIndexToFile(&indexMap, indexKey, kFaissFlatIndex);
}

bool VectorIndex::updateFlatIndex(int d, const QVector<float> &embeddings, const QVector<faiss::idx_t> &ids, const QString &indexKey)
{
    faiss::idx_t n = embeddings.count() / d;
    faiss::IndexIDMap *index = dynamic_cast<faiss::IndexIDMap*>(loadIndexFromFile(indexKey, kFaissFlatIndex));
    if (!index)
        return false;

    index->add_with_ids(n, embeddings.data(), ids.data());
    //index->add(n, embeddings.data());

    return saveIndexToFile(index, indexKey, kFaissFlatIndex);
}

bool VectorIndex::deleteFlatIndex(const QVector<faiss::idx_t> &deleteID, const QString &indexKey)
{
    faiss::IndexIDMap *index = dynamic_cast<faiss::IndexIDMap*>(loadIndexFromFile(indexKey, kFaissFlatIndex));
    if (!index)
        return false;
    faiss::IDSelectorArray idSelector(static_cast<size_t>(deleteID.size()), deleteID.data());
    index->remove_ids(idSelector);
    return saveIndexToFile(index, indexKey, kFaissFlatIndex);
}

bool VectorIndex::createIvfFlatIndex(int d, const QVector<float> &embeddings, const QString &indexKey)
{
    return false;
}

bool VectorIndex::updateIvfFlatIndex(int d, const QVector<float> &embeddings, const QString &indexKey)
{
    return false;
}

int VectorIndex::getIndexNTotal(const QString &indexKey)
{
    //获取当前索引中向量总数  XXX
    faiss::Index *index = loadIndexFromFile(indexKey, kFaissFlatIndex);
    if (!index)
        return -1;
    int nTotal = static_cast<int>(index->ntotal);

    return nTotal;
}

void VectorIndex::removeDupIndex(const faiss::Index *index, int topK, int DupK, QVector<faiss::idx_t> &nonDupIndex,
                                 const float *queryVector, QMap<float, bool> &seen)
{
    if (nonDupIndex.count() == topK)
        return;

    QVector<float> D1(topK);
    QVector<faiss::idx_t> I1(topK);
    index->search(1, queryVector, topK, D1.data(), I1.data());

    for (int i = 0; i < topK; i++) {
        if (!seen[D1[i]]) {
            if (nonDupIndex.count() == topK)
                return;
            seen[D1[i]] = true;
            nonDupIndex.push_back(I1[i]);
        }
    }
    DupK += topK - nonDupIndex.count();
    removeDupIndex(index, topK, DupK, nonDupIndex, queryVector, seen);
}

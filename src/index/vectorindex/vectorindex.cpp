// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vectorindex.h"
#include "../global_define.h"
#include "database/embeddatabase.h"

#include <QList>
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>

#include <faiss/IndexIVFPQ.h>
#include <faiss/index_io.h>
#include <faiss/index_factory.h>
#include <faiss/utils/random.h>
#include <faiss/IndexShards.h>
#include <faiss/IndexFlatCodes.h>

VectorIndex::VectorIndex(QSqlDatabase *db, QMutex *mtx, QObject *parent)
    :QObject (parent)
    , dataBase(db)
    , dbMtx(mtx)
{
}

bool VectorIndex::updateIndex(int d, const QMap<faiss::idx_t, QVector<float>> &embedVectorCache, const QString &indexKey)
{
    if (embedVectorCache.isEmpty())
        return false;

    if (!flatIndexHash.contains(indexKey)) {
        faiss::Index *index = faiss::index_factory(d, kFaissFlatIndex);
        faiss::IndexIDMap *flatIndexIDMap = new faiss::IndexIDMap(index);
        flatIndexHash.insert(indexKey, flatIndexIDMap);
    }
    faiss::IndexIDMap *flatIndexIDMapTmp = flatIndexHash.value(indexKey);

    int oldNTotal = flatIndexIDMapTmp->ntotal;
    QVector<float> embeddingsTmp;
    QVector<faiss::idx_t> idsTmp;
    for (faiss::idx_t i = oldNTotal; i < embedVectorCache.keys().count(); i++) {
        embeddingsTmp += embedVectorCache.value(i);
        idsTmp << i;
    }

    flatIndexIDMapTmp->add_with_ids(embedVectorCache.count() - oldNTotal, embeddingsTmp.data(), idsTmp.data());
    int newNTotal = flatIndexIDMapTmp->ntotal;

    qInfo() << "old total" << oldNTotal;
    qInfo() << "new total" << newNTotal;

    segmentIds += idsTmp;   //每个segment的索引所对应的IDs

    if (flatIndexIDMapTmp->ntotal >= 100) {
        // UOS-AI添加文档后在内存中，与已经落盘的区分开，手动操作落盘；整理索引碎片等操作。
        Q_EMIT indexDump(indexKey);
    }
    return true;
}

bool VectorIndex::saveIndexToFile(const faiss::Index *index, const QString &indexKey, const QString &indexType)
{
    if (!index || index->ntotal == 0) {
        return false;
    }

    qInfo() << "save faiss index...";
    QString indexDirStr = workerDir() + QDir::separator() + indexKey;
    QDir indexDir(indexDirStr);

    if (!indexDir.exists()) {
        if (!indexDir.mkpath(indexDirStr)) {
            qWarning() << indexKey << " directory isn't exists and can't create!";
            return false;
        }
    }
    QHash<QString, int> indexFilesNum = getIndexFilesNum(indexKey);
    QString indexName = indexType + "_" + QString::number(indexFilesNum.value(indexType)) + ".faiss";
    QString indexPath = indexDir.path() + QDir::separator() + indexName;
    qInfo() << "index file save to " + indexPath;

    QStringList insertStrs;
    for (faiss::idx_t id : segmentIds) {
        QString insert = "INSERT INTO " + QString(kEmbeddingDBIndexSegTable)
                + " (id, " + QString(kEmbeddingDBSegIndexTableBitSet)
                + ", " + QString(kEmbeddingDBSegIndexIndexName) + ") " + "VALUES ("
                + QString::number(id) + ", " + "1" + ", '" + indexName + "')";
        insertStrs << insert;
    }

    {
        //QString query = "SELECT id FROM " + QString(kEmbeddingDBMetaDataTable) + " ORDER BY id DESC LIMIT 1";
        QMutexLocker lk(dbMtx);
        EmbedDBVendorIns->commitTransaction(dataBase, insertStrs);
    }

    segmentIds.clear();

    try {
        faiss::write_index(index, indexPath.toStdString().c_str());
        return true;
    } catch (faiss::FaissException &e) {
        std::cerr << "Faiss error: " << e.what() << std::endl;
    }

    return true;
}

void VectorIndex::resetCacheIndex(int d, const QMap<faiss::idx_t, QVector<float> > &embedVectorCache, const QString &indexKey)
{
    if (!flatIndexHash.contains(indexKey)) {
        faiss::Index *index = faiss::index_factory(d, kFaissFlatIndex);
        faiss::IndexIDMap *flatIndexIDMap = new faiss::IndexIDMap(index);
        flatIndexHash.insert(indexKey, flatIndexIDMap);
    }
    faiss::IndexIDMap *flatIndexIDMapTmp = flatIndexHash.value(indexKey);

    QVector<float> embeddingsTmp;
    QVector<faiss::idx_t> idsTmp;
    for (auto id : embedVectorCache.keys()) {
        embeddingsTmp += embedVectorCache.value(id);
        idsTmp << id;
    }

    qInfo() << "111111---" << flatIndexIDMapTmp->ntotal;
    flatIndexIDMapTmp->reset();
    qInfo() << "222222---" << flatIndexIDMapTmp->ntotal;
    flatIndexIDMapTmp->add_with_ids(embedVectorCache.count(), embeddingsTmp.data(), idsTmp.data());
    qInfo() << "3333333---" << flatIndexIDMapTmp->ntotal;

    segmentIds.clear();
    segmentIds += idsTmp;   //每个segment的索引所对应的IDs


    if (flatIndexIDMapTmp->ntotal >= 100) {
        // UOS-AI添加文档后在内存中，与已经落盘的区分开，手动操作落盘；整理索引碎片等操作。
        Q_EMIT indexDump(indexKey);
    }
}

void VectorIndex::vectorSearch(int topK, const float *queryVector, const QString &indexKey,
                               QMap<float, faiss::idx_t> &cacheSearchRes, QMap<float, faiss::idx_t> &dumpSearchRes)
{
    //QMap<float, faiss::idx_t> searchResult;  <L2距离, ID> Map小到大排序 合并cache和dump两个结果
    if (indexKey == kSystemAssistantKey) {
       //TODO:区分社区版、专业版
        QString indexPath = QString(kSystemAssistantData) + ".faiss";
        faiss::Index *index;
        try {
            index = faiss::read_index(indexPath.toStdString().c_str());
        } catch (faiss::FaissException &e) {
            std::cerr << "Faiss error: " << e.what() << std::endl;
            return;
        }

        QVector<float> D1(topK);
        QVector<faiss::idx_t> I1(topK);
        index->search(1, queryVector, topK, D1.data(), I1.data());

        for (int id = 0; id < topK; id++) {
            if (I1[id] == -1 || D1[id] == 0.f)
                //faiss search -1 表示错误结果
                break;
            dumpSearchRes.insert(D1[id], I1[id]);
        }
        return;
    }

    //缓存向量检索
    qInfo() << "load faiss index from cache...";
    QVector<float> D1Cache(topK);
    QVector<faiss::idx_t> I1Cache(topK);
    if (flatIndexHash.contains(indexKey)) {
        faiss::Index *index = flatIndexHash.value(indexKey);
        index->search(1, queryVector, topK, D1Cache.data(), I1Cache.data());
    }

    for (int i = 0; i < topK; i++) {
        if (I1Cache[i] == -1 || D1Cache[i] == 0.f)
            //faiss search -1 表示错误结果
            break;
        cacheSearchRes.insert(D1Cache[i], I1Cache[i]);
    }

    //落盘的索引检索
    qInfo() << "load faiss index from dump...";
    QString indexDirStr = workerDir() + QDir::separator() + indexKey;
    QDir indexDir(indexDirStr);

    if (!indexDir.exists()) {
        if (!indexDir.mkpath(indexDirStr)) {
            qWarning() << indexKey << " directory isn't exists and can't create!";
            return;
        }
    }

    QHash<QString, int> indexFilesNum = getIndexFilesNum(indexKey);
    for (int i = 0; i < indexFilesNum.value(QString(kFaissFlatIndex)); i++) {
        QString name = QString(kFaissFlatIndex) + "_" + QString::number(i) + ".faiss";
        QString indexPath = indexDir.path() + QDir::separator() + name;

        faiss::Index *index;
        try {
            index = faiss::read_index(indexPath.toStdString().c_str());
        } catch (faiss::FaissException &e) {
            std::cerr << "Faiss error: " << e.what() << std::endl;
            return;
        }
        QVector<float> D1(topK);
        QVector<faiss::idx_t> I1(topK);
        index->search(1, queryVector, topK, D1.data(), I1.data());

        for (int id = 0; id < topK; id++) {
            if (I1[id] == -1 || D1[id] == 0.f)
                //faiss search -1 表示错误结果
                break;
            dumpSearchRes.insert(D1[id], I1[id]);
        }
    }

    //检索结果处理
    //TODO:检索结果后处理-去重、过于相近或远
}

void VectorIndex::doIndexDump(const QString &indexKey)
{
    saveIndexToFile(flatIndexHash.value(indexKey), indexKey, kFaissFlatIndex);

    flatIndexHash.remove(indexKey);
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

QHash<QString, int> VectorIndex::getIndexFilesNum(const QString &indexKey)
{
    QHash<QString, int> result;

    QString indexDirStr = workerDir() + QDir::separator() + indexKey;
    QDir indexDir(indexDirStr);
    if (!indexDir.exists()) {
        if (!indexDir.mkpath(indexDirStr)) {
            qWarning() << indexKey << " directory isn't exists and can't create!";
            return {};
        }
    }

    QFileInfoList fileList = indexDir.entryInfoList(QDir::Files);

    for (QString indexTypeKey : {kFaissFlatIndex, kFaissIvfFlatIndex, kFaissIvfPQIndex}) {
        int count = 0;
        for (const QFileInfo& fileInfo : fileList) {
            QString fileName = fileInfo.fileName();
            if (fileName.contains(indexTypeKey))
                count += 1;
            result.insert(indexTypeKey, count);
        }
    }
    return result;
}

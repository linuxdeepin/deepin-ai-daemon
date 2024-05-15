// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EMBEDDING_H
#define EMBEDDING_H

#include <QJsonObject>
#include <QObject>
#include <QVector>
#include <QHash>
#include <QMap>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QMutex>

#include <faiss/Index.h>

typedef QJsonObject (*embeddingApi)(const QStringList &texts, void *user);

class Embedding : public QObject
{
    Q_OBJECT
public:
    explicit Embedding(QSqlDatabase *db, QMutex *mtx, QObject *parent = nullptr);

    bool embeddingDocument(const QString &docFilePath, const QString &key);
    QVector<QVector<float>> embeddingTexts(const QStringList &texts);
    void embeddingQuery(const QString &query, QVector<float> &queryVector);


    //DB operate
    //bool clearAllDBTable(const QString &key);
    bool batchInsertDataToDB(const QStringList &inserQuery, const QString &key);
    int getDBLastID();
    void createEmbedDataTable();
    bool isDupDocument(const QString &key, const QString &docFilePath);

    void embeddingClear();
    QVector<float> getEmbeddingVector();
    QVector<faiss::idx_t> getEmbeddingIds();
    QMap<faiss::idx_t, QVector<float>> getEmbedVectorCache();
    QMap<faiss::idx_t, QPair<QString, QString>> getEmbedDataCache();

    QString loadTextsFromSearch(const QString &indexKey, int topK, const QMap<float, faiss::idx_t> &cacheSearchRes,
                                    const QMap<float, faiss::idx_t> &dumpSearchRes);

    inline void setEmbeddingApi(embeddingApi api, void *user) {
        onHttpEmbedding = api;
        apiData = user;
    }

    void deleteCacheIndex(const QStringList &files);
    void doIndexDump(const QString &key);
public slots:
    void onIndexCreateSuccess(const QString &key);
private:
    QStringList textsSpliter(QString &texts);
    void textsSplitSize(const QString &text, QStringList &splits, QString &over, int pos = 0);

    embeddingApi onHttpEmbedding = nullptr;
    void *apiData = nullptr;

    QVector<float> embeddingVector;
    QVector<faiss::idx_t> embeddingIds;
    QStringList insertSqlstrs;
    bool isStop = false;

    QVector<QString> sourcePaths;
    //QHash<faiss::idx_t, QString> dataCache;
    QMap<faiss::idx_t, QPair<QString, QString>> embedDataCache;
    QMap<faiss::idx_t, QVector<float>> embedVectorCache;

    QSqlDatabase *dataBase = nullptr;
    QMutex *dbMtx = nullptr;
};

#endif // EMBEDDING_H

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
    explicit Embedding(QSqlDatabase *db, QMutex *mtx, const QString &appID, QObject *parent = nullptr);

    bool embeddingDocument(const QString &docFilePath);
    bool embeddingDocumentSaveAs(const QString &docFilePath);
    QVector<QVector<float>> embeddingTexts(const QStringList &texts);
    void embeddingQuery(const QString &query, QVector<float> &queryVector);

    //DB operate
    bool batchInsertDataToDB(const QStringList &inserQuery);
    int getDBLastID();
    void createEmbedDataTable();
    bool isDupDocument(const QString &docFilePath);

    void embeddingClear();

    QMap<faiss::idx_t, QVector<float>> getEmbedVectorCache();
    QMap<faiss::idx_t, QPair<QString, QString>> getEmbedDataCache();

    QString loadTextsFromSearch(int topK, const QMap<float, faiss::idx_t> &cacheSearchRes,
                                    const QMap<float, faiss::idx_t> &dumpSearchRes);

    inline static QString workerDir()
    {
        static QString workerDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                + "/embedding";
        return workerDir;
    }

    inline void setEmbeddingApi(embeddingApi api, void *user) {
        onHttpEmbedding = api;
        apiData = user;
    }

    void deleteCacheIndex(const QStringList &files);
    bool doIndexDump();
    bool doSaveAsDoc(const QString &file);
    bool doDeleteSaveAsDoc(const QStringList &files);
private:
    QStringList textsSpliter(QString &texts);
    void textsSplitSize(const QString &text, QStringList &splits, QString &over, int pos = 0);
    QPair<QString, QString> getDataCacheFromID(const faiss::idx_t &id);
    QString saveAsDocPath(const QString &doc);

    embeddingApi onHttpEmbedding = nullptr;
    void *apiData = nullptr;

    QMap<faiss::idx_t, QPair<QString, QString>> embedDataCache;
    QMap<faiss::idx_t, QVector<float>> embedVectorCache;

    QSqlDatabase *dataBase = nullptr;
    QMutex *dbMtx = nullptr;

    QMutex embeddingMutex;

    QString appID;
};

#endif // EMBEDDING_H

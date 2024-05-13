// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EMBEDDING_H
#define EMBEDDING_H

#include <QJsonObject>
#include <QObject>
#include <QVector>
#include <QStandardPaths>

#include <faiss/Index.h>

typedef QJsonObject (*embeddingApi)(const QStringList &texts, void *user);

class Embedding : public QObject
{
    Q_OBJECT
public:
    explicit Embedding(QObject *parent = nullptr);    

    void embeddingDocument(const QString &docFilePath, const QString &key);
    void embeddingTexts(const QStringList &texts);
    void embeddingQuery(const QString &query, QVector<float> &queryVector);

    //DB operate
    bool clearAllDBTable(const QString &key);
    bool batchInsertDataToDB(const QStringList &inserQuery, const QString &key);
    int getDBLastID(const QString &key);
    void createEmbedDataTable(const QString &key);
    bool isDupDocument(const QString &key, const QString &docFilePath);

    void embeddingClear();
    QVector<float> getEmbeddingVector();
    QVector<faiss::idx_t> getEmbeddingIds();

    QStringList loadTextsFromIndex(const QVector<faiss::idx_t> &ids, const QString &indexKey);

    inline void setEmbeddingApi(embeddingApi api, void *user) {
        onHttpEmbedding = api;
        apiData = user;
    }
public slots:
    void onIndexCreateSuccess(const QString &key);

private:
    void init();
    QStringList textsSpliter(QString &texts);
    embeddingApi onHttpEmbedding = nullptr;
    void *apiData = nullptr;

    QVector<float> embeddingVector;
    QVector<faiss::idx_t> embeddingIds;
    QStringList insertSqlstrs;
    bool isStop = false;
};

#endif // EMBEDDING_H

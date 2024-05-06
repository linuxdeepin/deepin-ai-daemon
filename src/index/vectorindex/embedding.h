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

//template <typename T> class QList;
//typedef QHash<int, QList<float>> EmbeddingVector;

class Embedding : public QObject
{
    Q_OBJECT
public:
    explicit Embedding(QObject *parent = nullptr);    

    void embeddingDocument(const QString &docFilePath, const QString &key, bool isOverWrite = false);
    void embeddingTexts(const QStringList &texts);
    void embeddingQuery(const QString &query, QVector<float> &queryVector);

    //DB operate
    bool clearAllDBTable(const QString &key);
    bool batchInsertDataToDB(const QStringList &inserQuery, const QString &key);
    int getDBLastID(const QString &key);
    void createEmbedDataTable(const QString &key);
    bool isDupDocument(const QString &key, const QString &docFilePath);

    inline static QString workerDir()
    {
        static QString workerDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                + "/embedding";
        return workerDir;
    }
    void embeddingClear();
    QVector<float> getEmbeddingVector();
    QVector<faiss::idx_t> getEmbeddingIds();

    QStringList loadTextsFromIndex(const QVector<faiss::idx_t> &ids, const QString &indexKey);

private:
    void init();

    QStringList textsSpliter(const QString &texts);
    QJsonObject getDataInfo(const QString &key);
    void updateDataInfo(const QJsonObject &dataInfos, const QString &key);


    QJsonObject onHttpEmbedding(const QStringList &texts, bool isQuery = true);

    QVector<float> embeddingVector;
    QVector<faiss::idx_t> embeddingIds;
    bool isStop = false;
};

#endif // EMBEDDING_H

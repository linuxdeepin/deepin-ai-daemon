// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VECTORINDEX_H
#define VECTORINDEX_H

#include <QObject>
#include <QSharedPointer>
#include <QStandardPaths>

#include <faiss/Index.h>

class VectorIndex : public QObject
{
    Q_OBJECT

public:
    explicit VectorIndex(QObject *parent = nullptr);
    void init();

    void createIndex(int d, const QVector<float> &embeddings, const QVector<faiss::idx_t> &ids, const QString &indexKey);
    void updateIndex(int d, const QVector<float> &embeddings, const QVector<faiss::idx_t> &ids, const QString &indexKey);
    void deleteIndex(const QString &indexKey, const QVector<faiss::idx_t> &deleteID);
    void saveIndexToFile(const faiss::Index *index, const QString &indexKey, const QString &indexType="All");
    faiss::Index* loadIndexFromFile(const QString &indexKey, const QString &indexType="All");

    //DB Operate
    void createIndexSegTable(const QString &key);

    QVector<faiss::idx_t> vectorSearch(int topK, const float *queryVector, const QString &indexKey);
    QStringList loadTexts(const QVector<faiss::idx_t> &ids, const QString &indexKey);

    inline static QString workerDir()
    {
        static QString workerDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                + "/embedding";
        return workerDir;
    }

private:
    void createFlatIndex(int d, const QVector<float> &embeddings, const QVector<faiss::idx_t> &ids, const QString &indexKey);
    void updateFlatIndex(int d, const QVector<float> &embeddings, const QVector<faiss::idx_t> &ids, const QString &indexKey);
    void deleteFlatIndex(const QVector<faiss::idx_t> &deleteID, const QString &indexKey);


    void createIvfFlatIndex(int d, const QVector<float> &embeddings, const QString &indexKey);
    void updateIvfFlatIndex(int d, const QVector<float> &embeddings, const QString &indexKey);

    int getIndexNTotal(const QString &indexKey);

    void removeDupIndex(const faiss::Index *index, int topK, int DupK, QVector<faiss::idx_t> &nonDupIndex,
                        const float *queryVector, QMap<float, bool> &seen);
//    void processData(const faiss::idx_t );

};

#endif // VECTORINDEX_H

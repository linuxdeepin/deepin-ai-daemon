// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VECTORINDEX_H
#define VECTORINDEX_H

#include <QObject>
#include <QSharedPointer>
#include <QStandardPaths>
#include <QVector>

#include <faiss/Index.h>
#include <faiss/IndexFlat.h>
#include <faiss/IndexIDMap.h>

class VectorIndex : public QObject
{
    Q_OBJECT

public:
    explicit VectorIndex(QObject *parent = nullptr);
    void init();

    bool updateIndex(int d, const QMap<faiss::idx_t, QVector<float>> &embedVectorCache, const QString &indexKey);
    bool saveIndexToFile(const faiss::Index *index, const QString &indexKey, const QString &indexType="All");

    //DB Operate
    void createIndexSegTable(const QString &key);

    void resetCacheIndex(int d, const QMap<faiss::idx_t, QVector<float>> &embedVectorCache, const QString &indexKey);

    void vectorSearch(int topK, const float *queryVector, const QString &indexKey,
                      QMap<float, faiss::idx_t> &cacheSearchRes, QMap<float, faiss::idx_t> &dumpSearchRes);

    inline static QString workerDir()
    {
        static QString workerDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                + "/embedding";
        return workerDir;
    }

signals:
    void indexDump(const QString &indexKey);

private slots:
    void onIndexDump(const QString &indexKey);

private:
    void removeDupIndex(const faiss::Index *index, int topK, int DupK, QVector<faiss::idx_t> &nonDupIndex,
                        const float *queryVector, QMap<float, bool> &seen);

    QHash<QString, int> getIndexFilesNum(const QString &indexKey);

    QHash<QString, faiss::IndexIDMap*> flatIndexHash;
    QVector<faiss::idx_t> segmentIds;
};

#endif // VECTORINDEX_H

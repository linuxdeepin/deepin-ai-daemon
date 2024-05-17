// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VECTORINDEX_H
#define VECTORINDEX_H

#include <QObject>
#include <QSharedPointer>
#include <QStandardPaths>
#include <QVector>

#include <QSqlDatabase>
#include <QMutex>

#include <faiss/Index.h>
#include <faiss/IndexFlat.h>
#include <faiss/IndexIDMap.h>

class VectorIndex : public QObject
{
    Q_OBJECT

public:
    explicit VectorIndex(QSqlDatabase *db, QMutex *mtx, const QString &appID, QObject *parent = nullptr);
    bool updateIndex(int d, const QMap<faiss::idx_t, QVector<float>> &embedVectorCache);
    bool saveIndexToFile(const faiss::Index *index, const QString &indexType="All");

    //DB Operate
    void resetCacheIndex(int d, const QMap<faiss::idx_t, QVector<float>> &embedVectorCache);
    void vectorSearch(int topK, const float *queryVector, QMap<float, faiss::idx_t> &cacheSearchRes, QMap<float, faiss::idx_t> &dumpSearchRes);

    inline static QString workerDir()
    {
        static QString workerDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                + "/embedding";
        return workerDir;
    }

    void doIndexDump();
signals:
    void indexDump();
private:
    QHash<QString, int> getIndexFilesNum();

    faiss::IndexIDMap *cacheIndex = nullptr;
    QVector<faiss::idx_t> segmentIds;

    QSqlDatabase *dataBase = nullptr;
    QMutex *dbMtx = nullptr;

    QMutex vectorIndexMtx;

    QString appID;
};

#endif // VECTORINDEX_H

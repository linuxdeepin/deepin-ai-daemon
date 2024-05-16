// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VECTORWORKER_P_H
#define VECTORWORKER_P_H

#include "../vectorindex/embedding.h"
#include "../vectorindex/vectorindex.h"

#include <QObject>
#include <QStandardPaths>
#include <QSqlDatabase>
#include <QMutex>
#include <QThread>

class EmbeddingWorkerPrivate : public QObject
{
    Q_OBJECT
public:
    enum IndexType {
        CreateIndex,   //创建索引
        UpdateIndex,    //更新索引
        VectorSearch     //向量检索
    };

    explicit EmbeddingWorkerPrivate(QObject *parent = nullptr);

    void init();
    bool enableEmbedding(const QString &file);
    inline static QString workerDir()
    {
        static QString workerDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                + "/embedding";
        return workerDir;
    }
    QStringList embeddingPaths();

    bool updateIndex(const QStringList &files);
    bool deleteIndex(const QStringList &files);
    QString vectorSearch(const QString &query, int topK);

    QString indexDir();
    QStringList getIndexDocs();

    bool isSupportDoc(const QString &file);
    bool isFilter(const QString &file);
public:
    Embedding *embedder {nullptr};
    VectorIndex *indexer {nullptr};

    bool m_creatingAll = false;

    QString indexKey;
    QThread workThread;

    QSqlDatabase dataBase;
    QMutex dbMtx;
};

#endif // VECTORWORKER_P_H

// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VECTORWORKER_P_H
#define VECTORWORKER_P_H

#include "../vectorindex/embedding.h"
#include "../vectorindex/vectorindex.h"

#include <QObject>
#include <QStandardPaths>
#include <QtConcurrent/QtConcurrent>

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

    bool createSystemAssistantIndex(const QString &indexKey);
    bool createAllIndex(const QStringList &files, const QString &indexKey);
    bool updateIndex(const QStringList &files, const QString &indexKey);
    bool deleteIndex(const QStringList &files, const QString &indexKey);
    QString vectorSearch(const QString &query, const QString &key, int topK);

    bool isIndexExists(const QString &indexKey);
    QString indexDir(const QString &indexKey);
    QStringList getIndexDocs(const QString &indexKey);

    bool isSupportDoc(const QString &file);

    Embedding *embedder {nullptr};
    VectorIndex *indexer {nullptr};

    bool m_creatingAll = false;
};

#endif // VECTORWORKER_P_H

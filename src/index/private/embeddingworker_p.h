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
        static QString workerDir = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                + "/embedding";
        return workerDir;
    }
    QStringList embeddingPaths();

    bool createSystemAssistantIndex(const QString &indexKey);
    bool createAllIndex(const QStringList &files, const QString &indexKey);
    bool updateIndex(const QStringList &files, const QString &indexKey);
    bool deleteIndex(const QStringList &files, const QString &indexKey, QVector<faiss::idx_t> deleteID);
    QStringList vectorSearch(const QString &query, const QString &key, int topK);

    bool isIndexExists(const QString &indexKey);
    QString indexDir(const QString &indexKey);

    Embedding *embedder {nullptr};
    VectorIndex *indexer {nullptr};

    //void doIndexTask(const VectorIndex &index, const QString &file, IndexType type);
//    void indexFile(Lucene::IndexWriterPtr writer, const QString &file, IndexType type);
//    bool checkUpdate(const Lucene::IndexReaderPtr &reader, const QString &file, IndexType &type);
//    Lucene::DocumentPtr indexDocument(const QString &file);
//    QList<AbstractPropertyParser::Property> fileProperties(const QString &file);

//    QMap<QString, AbstractPropertyParser *> propertyParsers;
//    quint32 indexFileCount { 0 };
//    std::atomic_bool isStoped { false };
};

#endif // VECTORWORKER_P_H

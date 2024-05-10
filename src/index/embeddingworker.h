// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EMBEDDINGWORKER_H
#define EMBEDDINGWORKER_H

#include "vectorindex/embedding.h"
#include "vectorindex/vectorindex.h"

#include <QObject>

class EmbeddingWorkerPrivate;
class EmbeddingWorker : public QObject
{
    Q_OBJECT
public:
    explicit EmbeddingWorker(QObject *parent = nullptr);
    ~EmbeddingWorker();

    void setEmbeddingApi(embeddingApi api, void *user);
    void stop();

    enum IndexCreateStatus {
        Failed = 0,
        Success = 1,
        Creating = 2
    };

public Q_SLOTS:
    //void onFileAttributeChanged(const QString &file);
    void onFileCreated(const QString &file);
    void onFileDeleted(const QString &file);

    bool doCreateIndex(const QStringList &files, const QString &key);
    bool doUpdateIndex(const QStringList &files, const QString &key);
    bool doDeleteIndex(const QStringList &files, const QString &key);
    QStringList doVectorSearch(const QString &query, const QString &key, int topK);

    bool indexExists(const QString &key);

    QStringList getDocFile(const QString &key);

signals:
    void statusChanged(const QStringList &files, IndexCreateStatus status, const QString &key);
    void indexCreateSuccess(const QString &key);
    void indexDeleted(const QStringList &files, const QString &key);

private:
    EmbeddingWorkerPrivate *d { nullptr };
};

#endif // EMBEDDINGWORKER_H

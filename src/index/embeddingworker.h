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
    void stop(const QString &key);

    void saveAllIndex(const QString &key);

    enum IndexCreateStatus {
        Failed = 0,
        Success = 1,
        Creating = 2
    };
    IndexCreateStatus createAllState();
    void setWatch(bool watch);
public Q_SLOTS:
    bool doCreateIndex(const QStringList &files, const QString &key);
    bool doDeleteIndex(const QStringList &files, const QString &key);

    void onFileMonitorCreate(const QString &file);
    void onFileMonitorDelete(const QString &file);

    void onCreateAllIndex();
    QString doVectorSearch(const QString &query, const QString &key, int topK);

    QStringList getDocFile(const QString &key);

signals:
    void statusChanged(const QString &key, const QStringList &files, IndexCreateStatus status);
    void indexCreateSuccess(const QString &key);
    void indexDeleted(const QString &key, const QStringList &files);

    void stopEmbedding(const QString &key);
private:
    void traverseAndCreate(const QString &path);
private:
    EmbeddingWorkerPrivate *d { nullptr };
};

#endif // EMBEDDINGWORKER_H

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
    explicit EmbeddingWorker(const QString &appid, QObject *parent = nullptr);
    ~EmbeddingWorker();

    void setEmbeddingApi(embeddingApi api, void *user);
    void stop();

    void saveAllIndex();

    enum IndexCreateStatus {
        Failed = 0,
        Success = 1,
        Creating = 2
    };
    IndexCreateStatus createAllState();
    void setWatch(bool watch);
public Q_SLOTS:
    QString doVectorSearch(const QString &query, int topK);
    QStringList getDocFile();

    void onCreateAllIndex();
    bool doCreateIndex(const QStringList &files);
    bool doDeleteIndex(const QStringList &files);
    void onFileMonitorCreate(const QString &file);
    void onFileMonitorDelete(const QString &file);
private Q_SLOTS:
    void doIndexDump();
//end

signals:
    void statusChanged(const QString &key, const QStringList &files, int status);
    void indexCreateSuccess(const QString &key);
    void indexDeleted(const QString &key, const QStringList &files);

    void stopEmbedding();
private:
    void traverseAndCreate(const QString &path);
private:
    EmbeddingWorkerPrivate *d { nullptr };
};

#endif // EMBEDDINGWORKER_H

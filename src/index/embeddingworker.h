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

public Q_SLOTS:
    //void onFileAttributeChanged(const QString &file);
    void onFileCreated(const QString &file);
    void onFileDeleted(const QString &file);

    void onUpdateAllIndex();

    bool doCreateIndex(const QStringList &files, const QString &key);
    bool doDeleteIndex(const QStringList &files, const QString &key);
    QStringList doVectorSearch(const QString &query, const QString &key, int topK);

private:
    EmbeddingWorkerPrivate *d { nullptr };
};

#endif // EMBEDDINGWORKER_H

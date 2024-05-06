// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef INDEXMANAGER_H
#define INDEXMANAGER_H

#include "indexworker.h"
#include "index/embeddingworker.h"

#include <QObject>
#include <QThread>
#include <QSharedPointer>

class IndexManager : public QObject
{
    Q_OBJECT
public:
    explicit IndexManager(QObject *parent = nullptr);
    ~IndexManager();

Q_SIGNALS:
    void createAllIndex();
    void fileAttributeChanged(const QString &file);
    void fileCreated(const QString &file);
    void fileDeleted(const QString &file);

private:
    void init();

private:
    QSharedPointer<QThread> workThread { nullptr };
    QSharedPointer<IndexWorker> worker { nullptr };

    QSharedPointer<QThread> embeddingWorkThread { nullptr };
    QSharedPointer<EmbeddingWorker> embeddingWorker { nullptr };
};

#endif   // INDEXMANAGER_H

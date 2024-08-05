// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef INDEXMANAGER_H
#define INDEXMANAGER_H

#include "indexworker.h"
#include "index/embeddingworker.h"
#include "modelhub/modelhubwrapper.h"

#include <QObject>
#include <QThread>
#include <QSharedPointer>

class IndexManager : public QObject
{
    Q_OBJECT
public:
    explicit IndexManager(QObject *parent = nullptr);
    ~IndexManager();
    static IndexManager *instance();
Q_SIGNALS:
    void createAllIndex();
    void fileAttributeChanged(const QString &file);
    void fileCreated(const QString &file);
    void fileDeleted(const QString &file);

public slots:
    void onSemanticAnalysisChecked(bool isChecked, bool isFromUser = true);

private:
    void init();

private:
    QSharedPointer<QThread> workThread { nullptr };
    QSharedPointer<IndexWorker> worker { nullptr };
    volatile bool isSemanticOn = false;
    QMutex semanticOnMutex;
};

#endif   // INDEXMANAGER_H

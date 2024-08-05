// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "indexmanager.h"
#include "../config/configmanager.h"

#include <QDebug>

static IndexManager *kIndexManager = nullptr;

IndexManager::IndexManager(QObject *parent)
    : QObject(parent),
      workThread(new QThread),
      worker(new IndexWorker)
{
    Q_ASSERT(kIndexManager == nullptr);
    kIndexManager = this;

    init();
}

IndexManager::~IndexManager()
{
    if (kIndexManager == this)
        kIndexManager = nullptr;

    if (workThread->isRunning()) {
        worker->stop();

        workThread->quit();
        workThread->wait();
    }

    qInfo() << "The index manager has quit";
}

IndexManager *IndexManager::instance()
{
    return kIndexManager;
}

void IndexManager::init()
{
    worker->moveToThread(workThread.data());
    workThread->start();
}

void IndexManager::onSemanticAnalysisChecked(bool isChecked, bool isFromUser) {
    qInfo() << QString("onSemanticAnalysisChecked(%1 => %2) isFromUser(%3)").arg(isSemanticOn).arg(isChecked).arg(isFromUser);
    QMutexLocker lock(&semanticOnMutex);
    if (isSemanticOn == isChecked) {
        return;
    }

    if (isChecked) {
        isSemanticOn = true;
        ConfigManagerIns->setValue(SEMANTIC_ANALYSIS_GROUP, ENABLE_SEMANTIC_ANALYSIS, isSemanticOn);
        connect(this, &IndexManager::createAllIndex, worker.data(), &IndexWorker::onCreateAllIndex);
        connect(this, &IndexManager::fileCreated, worker.data(), &IndexWorker::onFileCreated);
        connect(this, &IndexManager::fileAttributeChanged, worker.data(), &IndexWorker::onFileAttributeChanged);
        connect(this, &IndexManager::fileDeleted, worker.data(), &IndexWorker::onFileDeleted);
        worker->start();
        if (isFromUser) {
            lock.unlock();
            emit createAllIndex();
        }
        return;
    }

    worker->stop();
    disconnect(this, &IndexManager::createAllIndex, worker.data(), &IndexWorker::onCreateAllIndex);
    disconnect(this, &IndexManager::fileCreated, worker.data(), &IndexWorker::onFileCreated);
    disconnect(this, &IndexManager::fileAttributeChanged, worker.data(), &IndexWorker::onFileAttributeChanged);
    disconnect(this, &IndexManager::fileDeleted, worker.data(), &IndexWorker::onFileDeleted);
    isSemanticOn = false;
    ConfigManagerIns->setValue(SEMANTIC_ANALYSIS_GROUP, ENABLE_SEMANTIC_ANALYSIS, isSemanticOn);
}

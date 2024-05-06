// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "indexmanager.h"

#include <QDebug>

IndexManager::IndexManager(QObject *parent)
    : QObject(parent),
      workThread(new QThread),
      worker(new IndexWorker),
      embeddingWorkThread(new QThread),
      embeddingWorker(new EmbeddingWorker)
{
    init();
}

IndexManager::~IndexManager()
{
    if (workThread->isRunning()) {
        worker->stop();
        //TODO EmbeddingWorker stop

        workThread->quit();
        workThread->wait();

        embeddingWorkThread->quit();
        embeddingWorkThread->wait();
    }

    qInfo() << "The index manager has quit";
    qInfo() << "The vector index manager has quit";
}

void IndexManager::init()
{
    connect(this, &IndexManager::createAllIndex, worker.data(), &IndexWorker::onCreateAllIndex);
    connect(this, &IndexManager::fileCreated, worker.data(), &IndexWorker::onFileCreated);
    connect(this, &IndexManager::fileAttributeChanged, worker.data(), &IndexWorker::onFileAttributeChanged);
    connect(this, &IndexManager::fileDeleted, worker.data(), &IndexWorker::onFileDeleted);

    //connect(this, &IndexManager::createAllIndex, embeddingWorker.data(), &EmbeddingWorker::onCreateAllIndex);
    connect(this, &IndexManager::fileCreated, embeddingWorker.data(), &EmbeddingWorker::onFileCreated);
    connect(this, &IndexManager::fileDeleted, embeddingWorker.data(), &EmbeddingWorker::onFileDeleted);

    workThread->start();
    embeddingWorkThread->start();

    embeddingWorker->moveToThread(embeddingWorkThread.data());
    worker->moveToThread(workThread.data());
}

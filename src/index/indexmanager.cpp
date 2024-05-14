// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "indexmanager.h"

#include <QDebug>

static IndexManager *kIndexManager = nullptr;

IndexManager::IndexManager(QObject *parent)
    : QObject(parent),
      workThread(new QThread),
      worker(new IndexWorker)
{
    Q_ASSERT(kIndexManager == nullptr);
    kIndexManager = this;

    //init(); TODO:
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
    //connect(this, &IndexManager::createAllIndex, worker.data(), &IndexWorker::onCreateAllIndex);
    connect(this, &IndexManager::fileCreated, worker.data(), &IndexWorker::onFileCreated);
    connect(this, &IndexManager::fileAttributeChanged, worker.data(), &IndexWorker::onFileAttributeChanged);
    connect(this, &IndexManager::fileDeleted, worker.data(), &IndexWorker::onFileDeleted);

    worker->moveToThread(workThread.data());
    workThread->start();
}


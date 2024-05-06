// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "analyzeserver.h"
#include "analyzeserverdbus.h"
#include "analyzeserveradaptor.h"
#include "vectorindexadaptor.h"

#include <QDBusConnection>

void AnalyzeServerDBusWorker::launchService()
{
    Q_ASSERT(QThread::currentThread() != qApp->thread());
    auto connAs { QDBusConnection::sessionBus() };
    if (!connAs.registerService("org.deepin.ai.daemon.AnalyzeServer")) {
        qWarning("Cannot register the \"org.deepin.ai.daemon.AnalyzeServer\" service.\n");
        return;
    }

    qInfo() << "Init DBus AnalyzeServer start";
    asDBus.reset(new AnalyzeServerDBus);
    Q_UNUSED(new AnalyzeServerAdaptor(asDBus.data()));
    if (!connAs.registerObject("/org/deepin/ai/daemon/AnalyzeServer",
                             asDBus.data())) {
        qWarning("Cannot register the \"/org/deepin/ai/daemon/AnalyzeServer\" object.\n");
        asDBus.reset(nullptr);
    }

    qInfo() << "Init DBus AnalyzeServer end";


    auto connVi { QDBusConnection::sessionBus() };
    if (!connVi.registerService("org.deepin.ai.daemon.VectorIndex")) {
        qWarning("Cannot register the \"org.deepin.ai.daemon.VectorIndex\" service.\n");
        return;
    }

    qInfo() << "Init DBus VectorIndex start";
    viDBus.reset(new VectorIndexDBus);
    Q_UNUSED(new VectorIndexAdaptor(viDBus.data()));
    if (!connVi.registerObject("/org/deepin/ai/daemon/VectorIndex",
                             viDBus.data())) {
        qWarning("Cannot register the \"/org/deepin/ai/daemon/VectorIndex\" object.\n");
        viDBus.reset(nullptr);
    }

    qInfo() << "Init DBus VecotrIndex end";
}

AnalyzeServer::AnalyzeServer()
{
    init();
}

AnalyzeServer::~AnalyzeServer()
{
    workerThread.quit();
    workerThread.wait();
    qInfo() << "The analyze service has quit";
}

void AnalyzeServer::start()
{
    QDBusConnection connection = QDBusConnection::sessionBus();
    if (!connection.registerService("org.deepin.ai.daemon")) {
        qCritical("Cannot register the \"org.deepin.ai.daemon\" service!!!\n");
        ::exit(EXIT_FAILURE);
    }

    emit requestLaunch();
}

void AnalyzeServer::init()
{
    AnalyzeServerDBusWorker *worker { new AnalyzeServerDBusWorker };
    worker->moveToThread(&workerThread);
    connect(&workerThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &AnalyzeServer::requestLaunch, worker, &AnalyzeServerDBusWorker::launchService);
    workerThread.start();
}

// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vectorindexdbus.h"

#include <QCoreApplication>
#include <QDebug>
#include <QThread>
#include <QDBusConnection>

VectorIndexWorker::VectorIndexWorker(QObject *parent)
    : QObject(parent)
{
    process.setProgram("deepin-ai-vectorindex");
}

void VectorIndexWorker::stop()
{
    if (process.state() == QProcess::Running)
        process.kill();
}

void VectorIndexWorker::onTaskAdded(const QString &content, QDBusMessage reply)
{
    QString result;
    process.setArguments({ content });
    process.start();
    if (!process.waitForFinished()) {
        qWarning() << "deepin-ai-vectorindex execute failed: "
                   << process.errorString();
        goto end;
    }

    if (process.exitCode() != 0) {
        qWarning() << process.readAllStandardError();
        goto end;
    }

    result = process.readAllStandardOutput();
    qDebug() << result;
end:
    reply << result;
    QDBusConnection::sessionBus().send(reply);
}

VectorIndexDBus::VectorIndexDBus(QObject *parent) : QObject(parent)
{
    init();
}

VectorIndexDBus::~VectorIndexDBus()
{
    worker->stop();
    workerThread.quit();
    workerThread.wait();

    if (worker)
        delete worker;

    if (ew)
        delete ew;
}

bool VectorIndexDBus::Create(const QStringList &files, const QString &key)
{
    qInfo() << "Index Create!";
    return ew->doCreateIndex(files, key);
}

bool VectorIndexDBus::Delete(const QStringList &files, const QString &key)
{
    qInfo() << "Index Delete!";
    return ew->doDeleteIndex(files, key);
}

QStringList VectorIndexDBus::Search(const QString &query, const QString &key, int topK)
{
    qInfo() << "Index Search!";
    return  ew->doVectorSearch(query, key, topK);
}

void VectorIndexDBus::init()
{
    ew = new EmbeddingWorker(this);

    worker = new VectorIndexWorker();
    worker->moveToThread(&workerThread);

    //connect(this, &VectorIndexDBus::addTask, worker, &VectorIndexWorker::onTaskAdded);
    workerThread.start();
}

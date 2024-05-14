// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "analyzeserverdbus.h"
#include "modelhub/modelhubwrapper.h"

#include <QProcess>
#include <QDebug>
#include <QDBusConnection>
#include <QFileInfo>

AnalyzeWorker::AnalyzeWorker(QObject *parent)
    : QObject(parent)
{
    process.setProgram("deepin-ai-models");
}

void AnalyzeWorker::stop()
{
    if (process.state() == QProcess::Running)
        process.kill();
}

void AnalyzeWorker::onTaskAdded(const QString &content, QDBusMessage reply)
{
    QString result;
    process.setArguments({ content });
    process.start();
    if (!process.waitForFinished()) {
        qWarning() << "deepin-ai-models execute failed: "
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

AnalyzeServerDBus::AnalyzeServerDBus(QObject *parent)
    : QObject(parent)
{
    init();
}

AnalyzeServerDBus::~AnalyzeServerDBus()
{
    worker->stop();
    workerThread.quit();
    workerThread.wait();

    if (worker)
        delete worker;
}

QString AnalyzeServerDBus::Analyze(const QString &content)
{
    if (content.isEmpty())
        return "";

    auto msg = message();
    msg.setDelayedReply(true);
    auto reply = msg.createReply();

    worker->stop();
    Q_EMIT addTask(content, reply, {});
    return "";
}

bool AnalyzeServerDBus::Enable()
{
    QString out;
    ModelhubWrapper::openCmd("which deepin-ai-models", out);
    return out.contains("deepin-ai-models");
}

void AnalyzeServerDBus::init()
{
    worker = new AnalyzeWorker();
    worker->moveToThread(&workerThread);

    connect(this, &AnalyzeServerDBus::addTask, worker, &AnalyzeWorker::onTaskAdded);
    workerThread.start();
}

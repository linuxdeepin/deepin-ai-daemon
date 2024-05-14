// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "modelhubwrapper.h"

#include <QString>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QThread>
#include <QEventLoop>
#include <QFileInfo>
#include <QProcess>

#include <unistd.h>

ModelhubWrapper::ModelhubWrapper(const QString &model, QObject *parent)
    : QObject(parent)
    , modelName(model)
{
    Q_ASSERT(!model.isEmpty());
}

ModelhubWrapper::~ModelhubWrapper()
{
    if (pid > 0) {
        int rpid = modelStatus(modelName).value("pid").toInt();

        // start by me, to kill.
        if (rpid == pid) {
            qInfo() << "kill server" << modelName << pid;
            system(QString("kill -3 %0").arg(pid).toStdString().c_str());
        } else {
            qInfo() << "server" << modelName << pid << "is not launched by me.";
        }
    }
}

bool ModelhubWrapper::isRunning()
{
    QString statFile = QString("/tmp/deepin-modelhub-%0/%1.state").arg(getuid()).arg(modelName);
    bool ret = QFileInfo::exists(statFile);
    return ret;
}

bool ModelhubWrapper::ensureRunning()
{
    if (health())
        return true;

    // check running by user
    {
        updateHost();
        if (!host.isEmpty() && port > 0)
            return true;
    }

    const int idle = 180;
    int cur = QDateTime::currentSecsSinceEpoch();
    if (QDateTime::currentSecsSinceEpoch() - started < idle * 1000) {
        qWarning() << "can not start" << modelName << "last try" << started;
        return false;
    }

    qDebug() << "start modelhub server" << modelName;

    QProcess process;
    process.setProgram("deepin-modelhub");
    process.setArguments({"--run", modelName, "--exit-idle", QString::number(idle)}); // 3分钟自动退出
    bool ok = process.startDetached(&pid);
    started = cur;
    if (!ok || pid < 1) {
        qWarning() << "fail to start" << process.program() << process.arguments()
                   << process.environment();
        return false;
    }

    qInfo() << modelName << "server start" << process.pid();

    // wait server
    {
        const QString proc = QString("/proc/%0").arg(pid);
        int waitCount = 60 * 5; // 等1分钟加载模型
        while (waitCount-- && QFileInfo::exists(proc)) {
            QThread::msleep(200);
            updateHost();
            if (!host.isEmpty() && port > 0) {
                qInfo() << modelName << process.pid() << "get server host" << host << port;
                return true;
            }
        }
    }

    return false;
}

bool ModelhubWrapper::health()
{
    if (host.isEmpty() || port < 1)
        return false;

    QUrl url = urlPath("/health");

    QNetworkAccessManager manager;
    QNetworkRequest request(url);

    QNetworkReply *reply = manager.get(request);
    if (!reply)
        return false;

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    reply->deleteLater();
    return reply->error() == QNetworkReply::NoError;
}

QString ModelhubWrapper::urlPath(const QString &api) const
{
    return QString("http://%0:%1%2").arg(host).arg(port).arg(api);
}

bool ModelhubWrapper::isModelhubInstalled()
{
    QString out;
    if (openCmd("deepin-modelhub -v", out))
        return true;

    return false;
}

bool ModelhubWrapper::isModelInstalled(const QString &model)
{
    if (model.isEmpty())
        return false;

    QString out;
    if (openCmd("deepin-modelhub --list model", out)) {
        auto vh = QJsonDocument::fromJson(out.toUtf8()).object().toVariantHash();
        return vh.value("model").toStringList().contains(model);
    }

    return false;
}

QVariantHash ModelhubWrapper::modelStatus(const QString &model)
{
    if (model.isEmpty())
        return {};

    QString out;
    if (openCmd("deepin-modelhub --list server --info", out)) {
        auto vh = QJsonDocument::fromJson(out.toUtf8()).object().toVariantHash();
        auto ml = vh.value("serverinfo").value<QVariantList>();
        for (const QVariant &var: ml) {
            auto mvh = var.value<QVariantHash>();
            if (mvh.value("model").toString().compare(out, Qt::CaseInsensitive)){
                return mvh;
            }
        }
    }

    return {};
}

bool ModelhubWrapper::openCmd(const QString &cmd, QString &out)
{
    FILE* pipe = popen(cmd.toStdString().c_str(), "r");
    if (!pipe) {
        qCritical() << "fail to popen" << cmd;
        return false;
    }

    char buffer[256] = {0};
    bool ok = fgets(buffer, sizeof(buffer), pipe) != nullptr;
    if (!ok) {
        pclose(pipe);
        qWarning() << "no output for popen" << cmd;
        return false;
    }

    pclose(pipe);

    out = QString(buffer);
    return true;
}

void ModelhubWrapper::updateHost()
{
    auto vh = modelStatus(modelName);
    host = vh.value("host").toString();
    port = vh.value("port").toInt();
}

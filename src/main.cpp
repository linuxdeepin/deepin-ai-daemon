// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "monitor/filemonitor.h"
#include "server/analyzeserver.h"
#include "config/configmanager.h"
#include "utils/resourcemanager.h"

#include <DApplication>
#include <DLog>

#include <signal.h>
#include <unistd.h>

DWIDGET_USE_NAMESPACE
DCORE_USE_NAMESPACE

static void appExitHandler(int sig)
{
    qInfo() << "signal" << sig << "exit.";

    // 释放资源
    qApp->quit();
}

static void initLog()
{
    // 设置终端和文件记录日志
    const QString logFormat =
            "%{time}{yyyyMMdd.HH:mm:ss.zzz}[%{type:1}][%{function:-35} %{line:-4} "
            "%{threadid} ] %{message}\n";
    DLogManager::setLogFormat(logFormat);
    DLogManager::registerConsoleAppender();
    DLogManager::registerFileAppender();
}

int main(int argc, char *argv[])
{
    // 安全退出
    signal(SIGINT, appExitHandler);
    signal(SIGQUIT, appExitHandler);
    signal(SIGTERM, appExitHandler);

    DApplication app(argc, argv);
    // 设置应用信息
    app.setOrganizationName("deepin");
    app.setApplicationName("deepin-ai-daemon");

    initLog();

    ConfigManagerIns->init();

    FileMonitor monitor;
    monitor.start(QThread::InheritPriority, 1);

    AnalyzeServer analyzeServer;
    analyzeServer.start();

    ResourceManager::instance()->setAutoReleaseMemory(true);

    return app.exec();
}

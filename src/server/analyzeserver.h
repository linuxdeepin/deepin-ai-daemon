// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ANALYZESERVER_H
#define ANALYZESERVER_H

#include <QThread>

class AnalyzeServerDBus;
class AnalyzeServerDBusWorker : public QObject
{
    Q_OBJECT
public Q_SLOTS:
    void launchService();

private:
    QScopedPointer<AnalyzeServerDBus> asDBus;
};

class AnalyzeServer : public QObject
{
    Q_OBJECT
public:
    explicit AnalyzeServer();
    ~AnalyzeServer();

    void start();

Q_SIGNALS:
    void requestLaunch();

private:
    void init();

private:
    QThread workerThread;
};

#endif   // ANALYZESERVER_H

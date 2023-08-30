// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ANALYZESERVERDBUS_H
#define ANALYZESERVERDBUS_H

#include <QDBusContext>
#include <QThread>
#include <QProcess>
#include <QDBusMessage>

class AnalyzeWorker : public QObject
{
    Q_OBJECT
public:
    explicit AnalyzeWorker(QObject *parent = nullptr);

    void stop();

public Q_SLOTS:
    void onTaskAdded(const QString &content, QDBusMessage reply);

private:
    QProcess process;
};

class AnalyzeServerDBus : public QObject, public QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.ai.daemon.AnalyzeServer")

public:
    explicit AnalyzeServerDBus(QObject *parent = nullptr);
    ~AnalyzeServerDBus();

public Q_SLOTS:
    QString Analyze(const QString &content);

Q_SIGNALS:
    void addTask(const QString &content, QDBusMessage reply, QPrivateSignal);

private:
    void init();

    AnalyzeWorker *worker { nullptr };
    QThread workerThread;
};

#endif   // ANALYZESERVERDBUS_H

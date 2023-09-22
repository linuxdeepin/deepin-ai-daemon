// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef MEMORYMANAGER_H
#define MEMORYMANAGER_H

#include <QTimer>

class ResourceManager : public QObject
{
    Q_OBJECT
public:
    static ResourceManager *instance();

    void setAutoReleaseMemory(bool enable);
    bool autoReleaseMemory();

private Q_SLOTS:
    void autoReleaseResource();

private:
    explicit ResourceManager(QObject *parent = nullptr);
    void init();

    void releaseMemory();
    double getMemoryUsage(int pid);

private:
    QTimer autoTimer;
    bool enableReleaseMem = false;
};

#endif   // MEMORYMANAGER_H

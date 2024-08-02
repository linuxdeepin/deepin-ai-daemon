// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef INDEXWORKER_H
#define INDEXWORKER_H

#include <QObject>

class IndexWorkerPrivate;
class IndexWorker : public QObject
{
    Q_OBJECT
public:
    explicit IndexWorker(QObject *parent = nullptr);
    void start();
    void stop();

public Q_SLOTS:
    void onFileAttributeChanged(const QString &file);
    void onFileCreated(const QString &file);
    void onFileDeleted(const QString &file);
    void onCreateAllIndex();
    void onUpdateAllIndex();

private:
    IndexWorkerPrivate *d { nullptr };
};

#endif   // INDEXWORKER_H

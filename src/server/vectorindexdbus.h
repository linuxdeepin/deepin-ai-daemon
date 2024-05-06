// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VECTORINDEXDBUS_H
#define VECTORINDEXDBUS_H

#include "index/embeddingworker.h"

#include <QObject>
#include <QDBusMessage>
#include <QProcess>
#include <QThread>

class VectorIndexWorker : public QObject
{
    Q_OBJECT
public:
    explicit VectorIndexWorker(QObject *parent = nullptr);

    void stop();

public Q_SLOTS:
    void onTaskAdded(const QString &content, QDBusMessage reply);

private:
    QProcess process;
};


class VectorIndexDBus : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.ai.daemon.VectorIndex")
public:
    explicit VectorIndexDBus(QObject *parent = nullptr);
    ~VectorIndexDBus();

public Q_SLOTS:
    bool Create(const QStringList &files, const QString &key);
    bool Delete(const QStringList &files, const QString &key);

    QStringList Search(const QString &query, const QString &key, int topK);

private:
    void init();

    VectorIndexWorker *worker { nullptr };
    QThread workerThread;

    EmbeddingWorker *ew {nullptr};
};

#endif // VECTORINDEXDBUS_H

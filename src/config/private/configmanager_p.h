// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CONFIGMANAGER_P_H
#define CONFIGMANAGER_P_H

#include <QObject>
#include <QFileSystemWatcher>
#include <QTimer>
#include <QVariantHash>
#include <QReadWriteLock>

class ConfigManagerPrivate : public QObject
{
    Q_OBJECT
public:
    explicit ConfigManagerPrivate(QObject *parent = nullptr);

    QVariant value(const QString &group, const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &group, const QString &key, const QVariant &value);

    void setDefaultConfig();
    void update();

    QHash<QString, QVariantHash> configs;
    QString configPath;
    QFileSystemWatcher *configWatcher { nullptr };
    QTimer delayLoadTimer;
    mutable QReadWriteLock mutex;
};

#endif   // CONFIGMANAGER_P_H

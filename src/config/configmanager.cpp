// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "configmanager.h"
#include "private/configmanager_p.h"

#include <QStandardPaths>
#include <QApplication>
#include <QFileInfo>
#include <QSettings>
#include <QDir>
#include <QDebug>

ConfigManagerPrivate::ConfigManagerPrivate(QObject *parent)
    : QObject(parent)
{
}

QVariant ConfigManagerPrivate::value(const QString &group, const QString &key, const QVariant &defaultValue) const
{
    QReadLocker rlk(&mutex);
    return configs.value(group).value(key, defaultValue);
}

void ConfigManagerPrivate::setValue(const QString &group, const QString &key, const QVariant &value)
{
    {
        QReadLocker rlk(&mutex);
        if (!configs.contains(group)) {
            rlk.unlock();

            QWriteLocker wlk(&mutex);
            configs.insert(group, { { key, value } });
            return;
        }
    }

    QWriteLocker wlk(&mutex);
    configs[group][key] = value;
}

void ConfigManagerPrivate::setDefaultConfig()
{
    QStringList blacklist {
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/WXWork",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/WeChat Files",
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Tencent Files"
    };

    QSettings set(configPath, QSettings::IniFormat);
    set.beginGroup(BLACKLIST_GROUP);
    set.setValue(BLACKLIST_PATHS, blacklist);
    set.endGroup();

    //添加默认的可Embedding的文件夹
    QStringList enableEmbeddingFiles {
        QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Embedding",
    };
    set.beginGroup(ENABLE_EMBEDDING_FILES_LIST_GROUP);
    set.setValue(ENABLE_EMBEDDING_PATHS, enableEmbeddingFiles);
    set.endGroup();
}

void ConfigManagerPrivate::update()
{
    QSettings set(configPath, QSettings::IniFormat);
    set.beginGroup(BLACKLIST_GROUP);
    const auto &blacklist = set.value(BLACKLIST_PATHS, QStringList()).toStringList();
    set.endGroup();

    set.beginGroup(AUTO_INDEX_GROUP);
    for (const QString &key : set.childKeys()) {
        setValue(AUTO_INDEX_GROUP, key, set.value(key, false).toBool());
    }
    set.endGroup();

    setValue(BLACKLIST_GROUP, BLACKLIST_PATHS, blacklist);
}

ConfigManager::ConfigManager(QObject *parent)
    : QObject(parent),
      d(new ConfigManagerPrivate(this))
{
}

void ConfigManager::init()
{
    d->delayLoadTimer.setSingleShot(true);
    d->delayLoadTimer.setInterval(50);
    connect(&d->delayLoadTimer, &QTimer::timeout, this, &ConfigManager::loadConfig);

    auto configPath = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation);
    d->configPath = configPath = configPath
            + "/" + qApp->organizationName()
            + "/" + qApp->applicationName()
            + "/" + qApp->applicationName() + ".conf";

    QFileInfo configFile(configPath);
    if (!configFile.exists()) {
        configFile.absoluteDir().mkpath(".");

        //生成文件
        QFile file(configPath);
        file.open(QFile::NewOnly);
        file.close();
        d->setDefaultConfig();
        qInfo() << "create conf " << configPath;
    }

    if (d->configWatcher)
        delete d->configWatcher;

    d->configWatcher = new QFileSystemWatcher(this);
    d->configWatcher->addPath(configFile.absolutePath());
    d->configWatcher->addPath(configFile.absoluteFilePath());
    connect(d->configWatcher, &QFileSystemWatcher::fileChanged, this, &ConfigManager::onFileChanged);
    connect(d->configWatcher, &QFileSystemWatcher::directoryChanged, this, &ConfigManager::onFileChanged);

    loadConfig();
}

ConfigManager *ConfigManager::instance()
{
    static ConfigManager ins;
    return &ins;
}

QVariant ConfigManager::value(const QString &group, const QString &key, const QVariant &defaultValue) const
{
    return d->value(group, key, defaultValue);
}

void ConfigManager::onFileChanged(const QString &file)
{
    qInfo() << "The configuration file changed: " << file;
    d->delayLoadTimer.start();
}

void ConfigManager::loadConfig()
{
    if (d->configPath.isEmpty())
        return;

    QFileInfo configFile(d->configPath);
    if (!configFile.exists())
        return;

    d->update();
}

void ConfigManager::setValue(const QString &group, const QString &key, bool value)
{
    QSettings set(d->configPath, QSettings::IniFormat);
    set.beginGroup(group);
    set.setValue(key, value);
    set.endGroup();
}

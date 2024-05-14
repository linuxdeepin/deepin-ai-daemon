// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QObject>
#include <QVariant>

#define BLACKLIST_GROUP "BlackList"
#define BLACKLIST_PATHS "Paths"

#define ENABLE_EMBEDDING_FILES_LIST_GROUP "EnableEmbeddingFilesList"
#define ENABLE_EMBEDDING_PATHS "Paths"

#define AUTO_INDEX_GROUP "AutoIndex"
#define AUTO_INDEX_STATUS "Status"

#define ConfigManagerIns ConfigManager::instance()

class ConfigManagerPrivate;
class ConfigManager : public QObject
{
    Q_OBJECT
public:
    static ConfigManager *instance();
    void init();

    QVariant value(const QString &group, const QString &key, const QVariant &defaultValue = QVariant()) const;
    void setValue(const QString &group, const QString &key, bool value);

protected Q_SLOTS:
    void onFileChanged(const QString &file);
    void loadConfig();    

private:
    explicit ConfigManager(QObject *parent = nullptr);

private:
    ConfigManagerPrivate *d { nullptr };
};

#endif   // CONFIGMANAGER_H

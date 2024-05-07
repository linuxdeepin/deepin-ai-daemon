// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EMBEDDATABASE_H
#define EMBEDDATABASE_H

#include <QObject>
#include <QThread>
#include <QtSql>

#define EmbedDBManagerIns EmbedDataBase::instance()

class EmbedDataBase : public QObject
{
    Q_OBJECT
public:
    static EmbedDataBase *instance();

    bool executeQuery(const QString &databaseName, const QString &queryStr, QList<QVariantMap> &result);
    bool executeQuery(const QString &databaseName, const QString &queryStr);
    bool commitTransaction(const QString &databaseName, const QStringList &queryList);

    bool isEmbedDataTableExists(const QString &databaseName, const QString &tableName);

private:
    explicit EmbedDataBase(QObject *parent = nullptr);

private:
    QSqlDatabase db;

    void init(const QString &databaseName);
    bool open();
    void close();
};

#endif // EMBEDDATABASE_H

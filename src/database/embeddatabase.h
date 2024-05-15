// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef EMBEDDATABASE_H
#define EMBEDDATABASE_H

#include <QObject>
#include <QThread>
#include <QtSql>
#include <QMutex>

#define EmbedDBVendorIns EmbedDBVendor::instance()

class EmbedDBVendor
{
public:
    static EmbedDBVendor *instance();
    QSqlDatabase addDatabase(const QString &databasePath);
    bool executeQuery(QSqlDatabase *db, const QString &queryStr, QList<QVariantMap> &result);
    bool executeQuery(QSqlDatabase *db, const QString &queryStr);
    bool commitTransaction(QSqlDatabase *db, const QStringList &queryList);
    bool isEmbedDataTableExists(QSqlDatabase *db, const QString &tableName);
protected:
    bool openDB(QSqlDatabase *db);
    void closeDB(QSqlDatabase *db);
private:
    explicit EmbedDBVendor();

};

#endif // EMBEDDATABASE_H

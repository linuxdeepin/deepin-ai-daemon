// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "embeddatabase.h"

#include <QTimer>
#include <QDebug>

EmbedDataBase::EmbedDataBase(QObject *parent)
    : QObject(parent)
{
    //init();
}

//EmbedDataBase::~EmbedDataBase()
//{
////    isStoped = true;

////    quit();
////    wait();
//    qInfo() << "The embedding dataBase has quit";
//}

void EmbedDataBase::init(const QString &databaseName)
{
    //打开数据库
    QString databasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() +  databaseName;
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(databasePath);
}

bool EmbedDataBase::isEmbedDataTableExists(const QString &databaseName, const QString &tableName)
{
    init(databaseName);

    QSqlQuery query(db);
    query.prepare("SELECT name FROM sqlite_master WHERE type='table' AND name=:tableName");
    query.bindValue(":tableName", tableName);
    if (!query.exec()) {
        qDebug() << "Error executing query:" << query.lastError().text();
        return false;
    }
    bool res = query.next();
    close();

    return res;
}

EmbedDataBase *EmbedDataBase::instance()
{
    static EmbedDataBase ins;
    return &ins;
}

bool EmbedDataBase::open()
{
    if (!db.open()) {
        qDebug() << "Failed to open database";
        return false;
    }
    qInfo() << "Openning DB Connected.";
    return true;
}

void EmbedDataBase::close()
{
    db.close();
    qInfo() << "Closing DB Connected";
}

bool EmbedDataBase::executeQuery(const QString &databaseName, const QString &queryStr, QList<QVariantMap> &result)
{
    init(databaseName);

    if (!open())
        return false;

    QSqlQuery query(db);
    if (query.exec(queryStr)) {
        while (query.next()) {
            QVariantMap res;
            res.insert("id", query.value(0));
            res.insert("source", query.value(1));
            res.insert("content", query.value(2));
            result.push_back(res);
        }
        close();
        return true;
    } else {
        qDebug() << "Error executing query:" << query.lastError().text();
        close();
        return false;
    }
}

bool EmbedDataBase::executeQuery(const QString &databaseName, const QString &queryStr)
{
    init(databaseName);

    if (!open())
        return false;

    QSqlQuery query(db);
    if (query.exec(queryStr)) {
        close();
        return true;
    } else {
        qDebug() << "Error executing query:" << query.lastError().text();
        close();
        return false;
    }
}

bool EmbedDataBase::commitTransaction(const QString &databaseName, const QStringList &queryList)
{
    init(databaseName);

    if (!open())
        return false;

    qInfo() << db.tables();

    QSqlQuery query(db);
    if (!query.exec("BEGIN TRANSACTION")) {
        qDebug() << "Failed to begin transaction";
        close();
        return false;
    }

    for (const QString &queryStr : queryList) {
        if (!query.exec(queryStr)) {
            qDebug() << "Error executing query:" << query.lastError().text();
            close();
            return false;
        }
    }

    if (!query.exec("COMMIT")) {
        qDebug() << "Failed to commit transaction";
        close();
        return 1;
    }

    close();

    return true;
}


//void EmbedDataBase::start(Priority p)
//{
//    QTimer::singleShot(0 * 1000, this, [&] {
//        if (!isInited) {
//            qWarning() << "database is not init success";
//            return;
//        }

//        //emit indexManager->createAllIndex();

//        QThread::start(p);
//    });
//}

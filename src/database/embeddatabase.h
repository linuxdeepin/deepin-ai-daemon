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
//    explicit EmbedDataBase(QObject *parent = nullptr, const QString &databaseName="");
//    virtual ~EmbedDataBase() override;

    bool executeQuery(const QString &databaseName, const QString &queryStr, QList<QVariantMap> &result);
    bool executeQuery(const QString &databaseName, const QString &queryStr);
    bool commitTransaction(const QString &databaseName, const QStringList &queryList);

    bool isEmbedDataTableExists(const QString &databaseName, const QString &tableName);

private:
    explicit EmbedDataBase(QObject *parent = nullptr);

//public Q_SLOTS:
//    void start(Priority = InheritPriority);



//protected:
//    void run() override;

private:
    QSqlDatabase db;

    void init(const QString &databaseName);
    bool open();
    void close();
    //bool isTableExists(const QString &tableName);


//    bool isInited { false };
//    std::atomic_bool isStoped { false };
};

#endif // EMBEDDATABASE_H

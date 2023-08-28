// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ABSTRACTPROPERTYPARSER_H
#define ABSTRACTPROPERTYPARSER_H

#include <QMap>
#include <QDateTime>
#include <QObject>

class AbstractPropertyParser : public QObject
{
    Q_OBJECT

public:
    struct Property
    {
        QString field;   // 字段名
        QString contents;   // 字段内容
        bool analyzed;   // 是否需要分词
    };

    explicit AbstractPropertyParser(QObject *parent = nullptr);
    virtual ~AbstractPropertyParser();

    virtual QList<Property> properties(const QString &file);

    QString formatTime(qint64 msec);
    QString formatTime(const QDateTime &time);
};

#endif   // ABSTRACTPROPERTYPARSER_H

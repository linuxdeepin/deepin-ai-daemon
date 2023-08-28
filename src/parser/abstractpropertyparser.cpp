// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "abstractpropertyparser.h"

#include <QFileInfo>

AbstractPropertyParser::AbstractPropertyParser(QObject *parent)
    : QObject(parent)
{
}

AbstractPropertyParser::~AbstractPropertyParser()
{
}

QList<AbstractPropertyParser::Property> AbstractPropertyParser::properties(const QString &file)
{
    QList<Property> propertyList;
    QFileInfo fileInfo(file);

    if (!fileInfo.exists())
        return propertyList;

    propertyList.append({ "path", file, false });
    propertyList.append({ "size", QString::number(fileInfo.size()), false });
    propertyList.append({ "created", formatTime(fileInfo.created()), false });
    propertyList.append({ "lastRead", formatTime(fileInfo.lastRead()), false });
    propertyList.append({ "lastModified", formatTime(fileInfo.lastModified()), false });
    propertyList.append({ "fileType", fileInfo.suffix(), false });

    return propertyList;
}

QString AbstractPropertyParser::formatTime(qint64 msec)
{
    auto hours = msec / (1000 * 60 * 60);
    int minutes = (msec % (1000 * 60 * 60)) / (1000 * 60);
    int seconds = ((msec % (1000 * 60 * 60)) % (1000 * 60)) / 1000;
    QString formattedDuration = QString("%1%2%3")
                                        .arg(hours, 2, 10, QLatin1Char('0'))
                                        .arg(minutes, 2, 10, QLatin1Char('0'))
                                        .arg(seconds, 2, 10, QLatin1Char('0'));
    return formattedDuration;
}

QString AbstractPropertyParser::formatTime(const QDateTime &time)
{
    return time.toString("yyyyMMddHHmmss");
}

// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "videopropertyparser.h"

#include <QLibrary>
#include <QJsonObject>
#include <QSize>
#include <QUrl>
#include <QDebug>

VideoPropertyParser::VideoPropertyParser(QObject *parent)
    : AbstractPropertyParser(parent)
{
    initLibrary();
}

QList<AbstractPropertyParser::Property> VideoPropertyParser::properties(const QString &file)
{
    auto propertyList = AbstractPropertyParser::properties(file);
    if (propertyList.isEmpty())
        return propertyList;

    QSize resolution;
    qint64 duration = 0;
    if (getMovieInfo(QUrl::fromLocalFile(file), resolution, duration)) {
        if (duration != -1)
            propertyList.append({ "duration", formatTime(duration * 1000), false });

        if (resolution != QSize { -1, -1 })
            propertyList.append({ "resolution", QString("%1*%2").arg(resolution.width()).arg(resolution.height()), false });
    }

    return propertyList;
}

void VideoPropertyParser::initLibrary()
{
    QLibrary imageLib("libimageviewer.so");
    if (!imageLib.load()) {
        qWarning() << "fail to load libimageviewer" << imageLib.errorString();
        return;
    }

    getMovieInfoFunc = reinterpret_cast<GetMovieInfoFunc>(imageLib.resolve("getMovieInfoByJson"));
    if (!getMovieInfoFunc)
        qWarning() << "can not find getMovieInfoByJson.";
}

bool VideoPropertyParser::getMovieInfo(const QUrl &url, QSize &dimension, qint64 &duration)
{
    if (!getMovieInfoFunc)
        return false;

    QJsonObject json;
    getMovieInfoFunc(url, &json);

    int ok = 0;
    if (json.contains("Base")) {
        const QJsonObject &base = json.value("Base").toObject();
        const QJsonValue &durationJson = base.value("Duration");
        if (durationJson.isString()) {
            QString timeStr = durationJson.toString();
            qDebug() << "get duration" << timeStr;
            QStringList times = timeStr.split(':');
            if (times.size() == 3) {
                duration = times.at(0).toInt() * 3600 + times.at(1).toInt() * 60 + times.at(2).toInt();
                ok = ok | 1;
            }
        }
        if (!(ok & 1)) {
            duration = -1;
            qDebug() << "get duration failed";
        }

        const QJsonValue &resolutionValue = base.value("Resolution");
        if (resolutionValue.isString()) {
            QString resolutionStr = resolutionValue.toString();
            qDebug() << "get resolution" << resolutionStr;
            QStringList strs = resolutionStr.split('x');
            if (strs.size() == 2) {
                dimension = QSize(strs.at(0).toInt(), strs.at(1).toInt());
                ok = ok | 2;
            }
        }

        if (!(ok & 2)) {
            dimension = QSize(-1, -1);
            qDebug() << "get dimension failed";
        }
    } else {
        return false;
    }

    return true;
}

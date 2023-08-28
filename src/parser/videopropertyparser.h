// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VIDEOPROPERTYPARSER_H
#define VIDEOPROPERTYPARSER_H

#include "abstractpropertyparser.h"

class VideoPropertyParser : public AbstractPropertyParser
{
public:
    explicit VideoPropertyParser(QObject *parent = nullptr);
    virtual QList<Property> properties(const QString &file) override;

private:
    void initLibrary();
    bool getMovieInfo(const QUrl &url, QSize &dimension, qint64 &duration);

private:
    typedef void (*GetMovieInfoFunc)(const QUrl &, QJsonObject *);
    GetMovieInfoFunc getMovieInfoFunc = nullptr;
};

#endif   //VIDEOPROPERTYPARSER_H

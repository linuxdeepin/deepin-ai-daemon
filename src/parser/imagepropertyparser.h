// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef IMAGEPROPERTYPARSER_H
#define IMAGEPROPERTYPARSER_H

#include "abstractpropertyparser.h"

#ifdef ENABLE_OCR
class ImagePropertyParser : public AbstractPropertyParser
{
public:
    explicit ImagePropertyParser(QObject *parent = nullptr);
    virtual QList<Property> properties(const QString &file) override;
};

#endif

#endif   //IMAGEPROPERTYPARSER_H

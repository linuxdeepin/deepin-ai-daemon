// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifdef ENABLE_OCR

#include "imagepropertyparser.h"

#include <DOcr>

#include <QImage>

static constexpr int kImageSizeLimit = 1024;

DOCR_USE_NAMESPACE
ImagePropertyParser::ImagePropertyParser(QObject *parent)
    : AbstractPropertyParser(parent)
{
}

QList<AbstractPropertyParser::Property> ImagePropertyParser::properties(const QString &file)
{
    auto propertyList = AbstractPropertyParser::properties(file);
    if (propertyList.isEmpty())
        return propertyList;

    QImage image(file);
    propertyList.append({ "resolution", QString("%1*%2").arg(image.width()).arg(image.height()), false });

    // 高分辨率图片会导致ocr内存暴涨
    if (image.width() > kImageSizeLimit || image.height() > kImageSizeLimit)
        image = image.width() > image.height()
                ? image.scaledToWidth(kImageSizeLimit, Qt::SmoothTransformation)
                : image.scaledToHeight(kImageSizeLimit, Qt::SmoothTransformation);

    DOcr ocr;
    ocr.loadDefaultPlugin();
    ocr.setLanguage("zh-Hans_en");
    ocr.setImage(image);
    if (ocr.analyze()) {
        const auto &result = ocr.simpleResult();
        if (!result.isEmpty())
            propertyList.append({ "contents", result, true });
    }

    return propertyList;
}

#endif

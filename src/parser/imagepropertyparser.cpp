// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "imagepropertyparser.h"

#include <QImage>

ImagePropertyParser::ImagePropertyParser(QObject *parent)
    : AbstractPropertyParser(parent)
{
    ocr.loadDefaultPlugin();
    ocr.setUseHardware({ { Dtk::Ocr::GPU_Vulkan, 0 } });
    ocr.setLanguage("zh-Hans_en");
}

QList<AbstractPropertyParser::Property> ImagePropertyParser::properties(const QString &file)
{
    auto propertyList = AbstractPropertyParser::properties(file);
    if (propertyList.isEmpty())
        return propertyList;

    QImage image(file);
    propertyList.append({ "resolution", QString("%1*%2").arg(image.width()).arg(image.height()), false });

    ocr.setImageFile(file);
    if (ocr.analyze()) {
        const auto &result = ocr.simpleResult();
        if (!result.isEmpty())
            propertyList.append({ "contents", result, true });
    }

    return propertyList;
}

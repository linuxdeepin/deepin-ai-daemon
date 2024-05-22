// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "utils.h"

#include <QTextStream>
#include <QTextCodec>
#include <QDebug>

#include <uchardet/uchardet.h>

Utils::Utils(QObject *parent) : QObject(parent)
{

}

QString Utils::textEncodingTransferUTF8(const std::string &content)
{
    if (content.empty())
        return {};

    QByteArray data = QByteArray::fromStdString(content);
    uchardet_t ud = uchardet_new();
    uchardet_handle_data(ud, data.constData(), static_cast<size_t>(data.size()));
    uchardet_data_end(ud);
    const char *codec = uchardet_get_charset(ud);
    if (codec == QString("UTF-8"))
        return QString::fromStdString(content);
    QTextCodec *oldCodec = QTextCodec::codecForName(codec);
    uchardet_delete(ud);

    QTextStream oldEncoding(data);
    oldEncoding.setCodec(oldCodec);

    return oldEncoding.readAll();
}

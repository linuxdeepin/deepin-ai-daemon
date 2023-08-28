// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef AUDIOPROPERTIESPARSER_H
#define AUDIOPROPERTIESPARSER_H

#include "abstractpropertyparser.h"

class AudioPropertyParser : public AbstractPropertyParser
{
    Q_OBJECT
public:
    struct AudioMetaData
    {
        QString title;
        QString artist;
        QString album;
        QString codec;
        QString duration;
    };

    explicit AudioPropertyParser(QObject *parent = nullptr);
    virtual QList<Property> properties(const QString &file) override;

private:
    bool openAudioFile(const QString &file, AudioMetaData &data);
    void characterEncodingTransform(AudioMetaData &meta, void *obj);
    QList<QByteArray> detectEncodings(const QByteArray &rawData);
    bool isChinese(const QChar &c);

private:
    QMap<QString, QByteArray> m_localeCodeMap;   // 区域与编码
};

#endif   // AUDIOPROPERTIESPARSER_H

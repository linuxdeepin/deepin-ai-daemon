// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef ANALYZESERVERDBUS_H
#define ANALYZESERVERDBUS_H

#include <QProcess>

class AnalyzeServerDBus : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.deepin.ai.daemon.AnalyzeServer")

public:
    explicit AnalyzeServerDBus(QObject *parent = nullptr);

public Q_SLOTS:
    QString Analyze(const QString &context);
};

#endif   // ANALYZESERVERDBUS_H

// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "analyzeserverdbus.h"

#include <QProcess>
#include <QDir>
#include <QDebug>

AnalyzeServerDBus::AnalyzeServerDBus(QObject *parent)
    : QObject(parent)
{
}

QString AnalyzeServerDBus::Analyze(const QString &context)
{
    QString result;
    if (context.isEmpty())
        return result;

    QProcess process;
    process.start("AImodule", { context });
    if (!process.waitForFinished()) {
        qWarning() << "AImodule execute failed: "
                   << process.errorString();
        return result;
    }

    if (process.exitCode() != 0) {
        qWarning() << process.readAllStandardError();
        return result;
    }

    result = process.readAllStandardOutput();
    qDebug() << result;
    return result;
}

#ifndef MODELHUBWRAPPER_H
#define MODELHUBWRAPPER_H
// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <QObject>
#include <QProcess>

class ModelhubWrapper : public QObject
{
    Q_OBJECT
public:
    explicit ModelhubWrapper(const QString &model, QObject *parent = nullptr);
    bool isRunning();
    bool ensureRunning();
    bool health();
    QString urlPath(const QString &api) const;
    static bool isModelhubInstalled();
    static bool isModelInstalled(const QString &model);
    static QVariantHash modelStatus(const QString &model);
protected:
    static bool openCmd(const QString &cmd, QString &out);
    void updateHost();
protected:
    QString modelName;
    QProcess process;
    QString host;
    int port = -1;
    int started = 0;
};
#endif   // MODELHUBWRAPPER_H

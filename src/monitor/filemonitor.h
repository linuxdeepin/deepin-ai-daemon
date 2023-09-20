// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef FILEMONITOR_H
#define FILEMONITOR_H

#include <QThread>

class IndexManager;
class FileMonitor : public QThread
{
    Q_OBJECT
public:
    explicit FileMonitor(QObject *parent = nullptr);
    virtual ~FileMonitor() override;

public Q_SLOTS:
    void start(Priority = InheritPriority, int delayTime = 0);

protected:
    void run() override;

private:
    void init();
    void initPolicy();
    bool addGroup(struct nl_sock *nlsock, const char *group);
    bool prepNlSock();
    QString pathRestore(const QString &filePath);

    static int handleMsgFromGenl(struct nl_msg *msg, void *arg);

private:
    IndexManager *indexManager { nullptr };
    struct nl_sock *nlsock { nullptr };
    struct nl_cb *nlcb { nullptr };
    bool isInited { false };
    std::atomic_bool isStoped { false };
};

#endif   // FILEMONITOR_H

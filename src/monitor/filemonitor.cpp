// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "filemonitor.h"
#include "index/indexmanager.h"
#include "vfsgenl.h"
#include "config/configmanager.h"
#include "index/global_define.h"
#include "config/configmanager.h"

#include <QDebug>
#include <QStandardPaths>
#include <QTimer>

#include <netlink/genl/genl.h>
#include <netlink/genl/ctrl.h>
#include <fstab.h>
#include <unistd.h>

#define get_attr(attrs, ATTR, attr, type) \
    if (!attrs[ATTR]) {                   \
        return 0;                         \
    }                                     \
    attr = nla_get_##type(attrs[ATTR])

/* major, minor*/
#define MKDEV(ma, mi) ((ma) << 8 | (mi))

/* attribute policy */
static struct nla_policy vfsnotify_genl_policy[VFSMONITOR_A_MAX + 1];

FileMonitor::FileMonitor(QObject *parent)
    : QThread(parent),
      indexManager(new IndexManager(this))
{
    init();
}

FileMonitor::~FileMonitor()
{
    isStoped = true;
    quit();
    wait();
    qInfo() << "The file monitor has quit";
}

void FileMonitor::start(Priority p, int delayTime)
{
    QTimer::singleShot(delayTime * 1000, this, [&] {
        if (!isInited) {
            qWarning() << "FileMonitor is not init success";
            return;
        }

        indexManager->onSemanticAnalysisChecked(ConfigManagerIns->value(SEMANTIC_ANALYSIS_GROUP, ENABLE_SEMANTIC_ANALYSIS, false).toBool());

        if (!prepNlSock())
            return;

        initPolicy();

        // set timeout
        struct timeval tv = { 0, 500 * 1000 };   // 500ms
        setsockopt(nl_socket_get_fd(nlsock), SOL_SOCKET, SO_RCVTIMEO, static_cast<void *>(&tv), sizeof(tv));

        QThread::start(p);
    });
}

void FileMonitor::run()
{
    qInfo() << "Start monitor files changes";
    while (!isStoped) {
        int ret = nl_recvmsgs_report(nlsock, nlcb);
        if (ret <= 0 && ret != -NLE_AGAIN)
            break;
    }

    nl_cb_put(nlcb);
    nl_socket_free(nlsock);
}

void FileMonitor::init()
{    
    if (!(nlsock = nl_socket_alloc())) {
        qWarning() << "nl_socket_alloc fail";
        return;
    }

    if (!(nlcb = nl_cb_alloc(NL_CB_DEFAULT))) {
        qWarning() << "nl_cb_alloc fail";
        return;
    }

    isInited = true;
}

void FileMonitor::initPolicy()
{
    vfsnotify_genl_policy[VFSMONITOR_A_ACT].type = NLA_U8;
    vfsnotify_genl_policy[VFSMONITOR_A_COOKIE].type = NLA_U32;
    vfsnotify_genl_policy[VFSMONITOR_A_MAJOR].type = NLA_U16;
    vfsnotify_genl_policy[VFSMONITOR_A_MINOR].type = NLA_U8;
    vfsnotify_genl_policy[VFSMONITOR_A_PATH].type = NLA_NUL_STRING;
    vfsnotify_genl_policy[VFSMONITOR_A_PATH].maxlen = 4096;
}

bool FileMonitor::addGroup(nl_sock *nlsock, const char *group)
{
    // 寻找广播地址
    int grpId = genl_ctrl_resolve_grp(nlsock, VFSMONITOR_FAMILY_NAME, group);
    if (grpId < 0) {
        qWarning() << "genl_ctrl_resolve_grp fail";
        return false;
    }

    // 加入广播
    if (nl_socket_add_membership(nlsock, grpId)) {
        qWarning() << "nl_socket_add_membership fail";
        return false;
    }

    return true;
}

bool FileMonitor::prepNlSock()
{
    int familyId;

    nl_socket_disable_seq_check(nlsock);
    nl_socket_disable_auto_ack(nlsock);

    /* connect to genl */
    if (genl_connect(nlsock)) {
        qWarning() << "genl_connect fail";
        return false;
    }

    /* resolve the generic nl family id*/
    familyId = genl_ctrl_resolve(nlsock, VFSMONITOR_FAMILY_NAME);
    if (familyId < 0) {
        qWarning() << "genl_ctrl_resolve fail";
        return false;
    }

    /* add group */
    if (!addGroup(nlsock, VFSMONITOR_MCG_DENTRY_NAME))
        return false;

    nl_cb_set(nlcb, NL_CB_VALID, NL_CB_CUSTOM, handleMsgFromGenl, this);
    return true;
}

QString FileMonitor::pathRestore(const QString &filePath)
{
    static QMap<QString, QString> table;
    static std::once_flag flag;
    std::call_once(flag, [] {
        struct fstab *fs;
        setfsent();
        while ((fs = getfsent()) != nullptr) {
            QString mntops(fs->fs_mntops);
            if (mntops.contains("bind"))
                table.insert(fs->fs_spec, fs->fs_file);
        }
        endfsent();
    });

    if (table.isEmpty())
        return filePath;

    QString tmpPath(filePath);
    for (const auto &device : table.keys()) {
        if (tmpPath.startsWith(device)) {
            tmpPath.replace(device, table[device]);
            break;
        }
    }

    return tmpPath;
}

int FileMonitor::handleMsgFromGenl(nl_msg *msg, void *arg)
{
    FileMonitor *monitor = static_cast<FileMonitor *>(arg);
    Q_ASSERT(monitor);

    if (monitor->isStoped)
        return 0;

    struct nlattr *attrs[VFSMONITOR_A_MAX + 1];
    unsigned char act;
    char *file = nullptr;
    unsigned int cookie;
    unsigned short major = 0;
    unsigned char minor = 0;
    int ret = genlmsg_parse(nlmsg_hdr(msg), 0, attrs, VFSMONITOR_A_MAX,
                            vfsnotify_genl_policy);
    if (ret < 0) {
        qWarning() << "error parse genl msg";
        return -1;
    }

    get_attr(attrs, VFSMONITOR_A_ACT, act, u8);
    get_attr(attrs, VFSMONITOR_A_COOKIE, cookie, u32);
    get_attr(attrs, VFSMONITOR_A_MAJOR, major, u16);
    get_attr(attrs, VFSMONITOR_A_MINOR, minor, u8);
    get_attr(attrs, VFSMONITOR_A_PATH, file, string);

    QString changedFile(file);
    if (changedFile.contains("/."))
        return 0;

    changedFile = monitor->pathRestore(changedFile);
    if (!changedFile.startsWith(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)))
        return 0;

    const auto &blacklist = ConfigManagerIns->value(BLACKLIST_GROUP, BLACKLIST_PATHS, QStringList()).toStringList();
    bool isFilter = std::any_of(blacklist.begin(), blacklist.end(), [&changedFile](const QString &path) {
        return changedFile.startsWith(path);
    });
    if (isFilter)
        return 0;

    // TODO: file attribute change
    switch (act) {
    case ACT_NEW_FILE:
        //    case ACT_NEW_SYMLINK:
        //    case ACT_NEW_LINK:
        //    case ACT_NEW_FOLDER:
    case ACT_RENAME_TO_FILE:
    case ACT_RENAME_TO_FOLDER:
        Q_EMIT monitor->indexManager->fileCreated(changedFile);
        break;
    case ACT_DEL_FILE:
    case ACT_DEL_FOLDER:
    case ACT_RENAME_FROM_FILE:
    case ACT_RENAME_FROM_FOLDER:
        Q_EMIT monitor->indexManager->fileDeleted(changedFile);
        break;
    default:
        break;
    }

    return 0;
}

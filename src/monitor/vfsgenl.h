// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VFS_GENL_H
#define VFS_GENL_H

#define ACT_NEW_FILE	        0
#define ACT_NEW_LINK	        1
#define ACT_NEW_SYMLINK	        2
#define ACT_NEW_FOLDER	        3
#define ACT_DEL_FILE	        4
#define	ACT_DEL_FOLDER	        5
#define ACT_RENAME_FILE		    6
#define ACT_RENAME_FOLDER	    7
#define ACT_RENAME_FROM_FILE	8
#define ACT_RENAME_TO_FILE	    9
#define ACT_RENAME_FROM_FOLDER	10
#define ACT_RENAME_TO_FOLDER	11
#define ACT_MOUNT	            12
#define ACT_UNMOUNT	            13

/* protocol family */
#define VFSMONITOR_FAMILY_NAME "vfsmonitor"

/* attributes */
enum {
    VFSMONITOR_A_UNSPEC,
    VFSMONITOR_A_ACT,
    VFSMONITOR_A_COOKIE,
    VFSMONITOR_A_MAJOR,
    VFSMONITOR_A_MINOR,
    VFSMONITOR_A_PATH,
    __VFSMONITOR_A_MAX,
};
#define VFSMONITOR_A_MAX (__VFSMONITOR_A_MAX - 1)

/* commands */
enum {
    VFSMONITOR_C_UNSPEC,
    VFSMONITOR_C_NOTIFY,
    __VFSMONITOR_C_MAX,
};
#define VFSMONITOR_C_MAX (__VFSMONITOR_C_MAX - 1)

/* multicast group */
#define VFSMONITOR_MCG_DENTRY_NAME VFSMONITOR_FAMILY_NAME "_de"

#endif

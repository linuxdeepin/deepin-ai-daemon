// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VECTORINDEXDBUS_H
#define VECTORINDEXDBUS_H

#include "index/embeddingworker.h"
#include "modelhub/modelhubwrapper.h"

#include <QObject>
#include <QDBusMessage>
#include <QProcess>
#include <QThread>

class VectorIndexDBus : public QObject
{
    Q_OBJECT    

    Q_CLASSINFO("D-Bus Interface", "org.deepin.ai.daemon.VectorIndex")
public:
    explicit VectorIndexDBus(QObject *parent = nullptr);
    ~VectorIndexDBus();
    static inline QString dependModel() {
        return QString("BAAI-bge-large-zh-v1.5");
    }

public Q_SLOTS:
    bool Create(const QStringList &files, const QString &key);
    bool Update(const QStringList &files, const QString &key);
    bool Delete(const QStringList &files, const QString &key);
    bool IndexExists(const QString &key);
    bool Enable();

    QStringList DocFiles(const QString &key);
    QStringList Search(const QString &query, const QString &key, int topK);

signals:
    void IndexStatus(const QStringList &files, int status, const QString &key);
    void IndexDeleted(const QStringList &files, const QString &key);

protected:
    static QJsonObject embeddingApi(const QStringList &texts, void *user);

private:
    EmbeddingWorker *ew {nullptr};
    ModelhubWrapper *bgeModel = nullptr;

    void init();
};

#endif // VECTORINDEXDBUS_H

// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef VECTORINDEXDBUS_H
#define VECTORINDEXDBUS_H

#include "index/embeddingworker.h"
#include "modelhub/modelhubwrapper.h"

#include <QObject>
#include <QDBusMessage>

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
    bool Delete(const QStringList &files, const QString &key);
    bool Enable();

    QStringList Search(const QString &query, const QString &key, int topK);

protected:
    static QJsonObject embeddingApi(const QStringList &texts, bool isQuery, void *user);
private:
    void init();

    EmbeddingWorker *ew {nullptr};
    ModelhubWrapper *bgeModel = nullptr;
};

#endif // VECTORINDEXDBUS_H

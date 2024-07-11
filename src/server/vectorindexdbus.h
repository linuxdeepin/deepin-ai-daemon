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
    bool Create(const QString &appID, const QStringList &files);
    bool Delete(const QString &appID, const QStringList &files);
    QString Search(const QString &appID, const QString &query, int topK);

    bool Enable();
    QString DocFiles(const QString &appID);

    QString getAutoIndexStatus(const QString &appID);
    void setAutoIndex(const QString &appID, bool on);

    void saveAllIndex(const QString &appID);

    void initBgeModel();

signals:
    void IndexStatus(const QString &appID, const QStringList &files, int status);
    void IndexDeleted(const QString &appID, const QStringList &files);

private:
    EmbeddingWorker *ensureWorker(const QString &appID);
protected:
    static QJsonObject embeddingApi(const QStringList &texts, void *user);

private:
    ModelhubWrapper *bgeModel = nullptr;
    QMap<QString, EmbeddingWorker*> embeddingWorkerwManager;
    QList<QString> m_whiteList;

    void init();
    void initEmbeddingWorker(EmbeddingWorker *ew);
};

#endif // VECTORINDEXDBUS_H

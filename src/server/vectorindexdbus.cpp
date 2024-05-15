// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vectorindexdbus.h"
#include "config/configmanager.h"
#include "index/global_define.h"

#include <QCoreApplication>
#include <QDebug>
#include <QThread>
#include <QDBusConnection>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDir>

VectorIndexDBus::VectorIndexDBus(QObject *parent) : QObject(parent)
{
    m_whiteList << kGrandVectorSearch;
    m_whiteList << "uos-ai-assistant";
    m_whiteList << kSystemAssistantKey;
    init();
}

VectorIndexDBus::~VectorIndexDBus()
{
    for (auto it : embeddingWorkerwManager) {
        delete it;
        it = nullptr;
    }
}

bool VectorIndexDBus::Create(const QString &appID, const QStringList &files)
{
    qInfo() << "Index Create!";
    EmbeddingWorker *embeddingWorker = ensureWorker(appID);
    if (!embeddingWorker)
        return false;

    return embeddingWorker->doCreateIndex(files, appID);
}

bool VectorIndexDBus::Delete(const QString &appID, const QStringList &files)
{
    qInfo() << "Index Delete!";
    EmbeddingWorker *embeddingWorker = ensureWorker(appID);
    if (!embeddingWorker)
        return false;

    return embeddingWorker->doDeleteIndex(files, appID);
}

bool VectorIndexDBus::Enable()
{
    return (bgeModel->isRunning()) || (ModelhubWrapper::isModelhubInstalled() &&
                                       ModelhubWrapper::isModelInstalled(dependModel()));
}

QStringList VectorIndexDBus::DocFiles(const QString &appID)
{
    EmbeddingWorker *embeddingWorker = ensureWorker(appID);
    if (!embeddingWorker)
        return {};

    return embeddingWorker->getDocFile(appID);
}

QString VectorIndexDBus::Search(const QString &appID, const QString &query, int topK)
{
    qInfo() << "Index Search!";
    EmbeddingWorker *embeddingWorker = ensureWorker(appID);
    if (!embeddingWorker)
        return "";

    return embeddingWorker->doVectorSearch(query, appID, topK);;
}

QString VectorIndexDBus::getAutoIndexStatus(const QString &appID)
{
    EmbeddingWorker *embeddingWorker = ensureWorker(appID);
    if (!embeddingWorker)
        return R"({"enable":false})";

    bool on = ConfigManagerIns->value(AUTO_INDEX_GROUP, appID + "." + AUTO_INDEX_STATUS, false).toBool();
    if (!on)
        return R"({"enable":false})";

    //
//    { enabel:true,
//      completion: 0 正在创建，1创建好了，-1创建失败
//      if 1
//        updatedtime: unix 时间戳 秒

//    }
    QVariantHash hash;
    hash.insert("enable", true);
    int st = embeddingWorker->createAllState() == EmbeddingWorker::Creating ? 0 : 1;
    hash.insert("completion", st);
    qint64 time = 0;
    {
        QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/embedding/" + appID);
        QFileInfoList fileList = dir.entryInfoList(QDir::Files, QDir::Time);
        if (!fileList.isEmpty()) {
            auto file = fileList.last();
            time = file.lastModified().toSecsSinceEpoch();
        }
    }

    if (st == 1)
        hash.insert("updatetime", time);

    QString str = QString::fromUtf8(QJsonDocument(QJsonObject::fromVariantHash(hash)).toJson());
    return str;
}

void VectorIndexDBus::setAutoIndex(const QString &appID, bool on)
{
    EmbeddingWorker *embeddingWorker = ensureWorker(appID);
    if (!embeddingWorker)
        return;

    ConfigManagerIns->setValue(AUTO_INDEX_GROUP, appID + "." + AUTO_INDEX_STATUS, on);
    if (on) {
        embeddingWorker->setWatch(false);
        embeddingWorker->onCreateAllIndex();
        embeddingWorker->setWatch(true);
    } else {
        embeddingWorker->stop(kGrandVectorSearch);
        embeddingWorker->setWatch(false);
    }
}

void VectorIndexDBus::saveAllIndex(const QString &appID)
{
    EmbeddingWorker *embeddingWorker = ensureWorker(appID);
    if (!embeddingWorker)
        return;
    Q_EMIT embeddingWorker->saveAllIndex(appID);
}

EmbeddingWorker *VectorIndexDBus::ensureWorker(const QString &appID)
{
    EmbeddingWorker *worker = embeddingWorkerwManager.value(appID);
    if (m_whiteList.contains(appID) && !worker) {
        worker = new EmbeddingWorker(this);
        initEmbeddingWorker(worker);
        embeddingWorkerwManager.insert(appID, worker);
    }

    return worker;
}

QJsonObject VectorIndexDBus::embeddingApi(const QStringList &texts, void *user)
{
    VectorIndexDBus *self = static_cast<VectorIndexDBus *>(user);
    if (!self->bgeModel->ensureRunning()) {
        return {};
    }

    QNetworkAccessManager manager;
    QNetworkRequest request(self->bgeModel->urlPath("/embeddings"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonArray jsonArray;
    for (const QString &str : texts) {
        jsonArray.append(str);
    }
    QJsonValue jsonValue = QJsonValue(jsonArray);

    QJsonObject data;
    data["input"] = jsonValue;

    QJsonDocument jsonDocHttp(data);
    QByteArray jsonDataHttp = jsonDocHttp.toJson();
    QJsonDocument replyJson;
    QNetworkReply *reply = manager.post(request, jsonDataHttp);

    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        replyJson = QJsonDocument::fromJson(response);
        qDebug() << "Response ok";
    } else {
        qDebug() << "Failed to create data:" << reply->errorString();
    }
    reply->deleteLater();
    QJsonObject obj = {};
    if (replyJson.isObject()) {
        obj = replyJson.object();
        return obj;
    }
    return {};
}

void VectorIndexDBus::init()
{
    if (ModelhubWrapper::isModelhubInstalled()) {
        if (!ModelhubWrapper::isModelInstalled(dependModel()))
            qWarning() << QString("VectorIndex needs model %0, but it is not avalilable").arg(dependModel());
    } else {
        qWarning() << "VectorIndex depends on deepin modehub, but it is not avalilable";
    }

    bgeModel = new ModelhubWrapper(dependModel(), this);

    for (const QString &app : m_whiteList) {
         bool on = ConfigManagerIns->value(AUTO_INDEX_GROUP, app + "." + AUTO_INDEX_STATUS, false).toBool();
         if (!on)
             continue;

         auto work = ensureWorker(app);
         work->setWatch(true);
    }
}

void VectorIndexDBus::initEmbeddingWorker(EmbeddingWorker *ew)
{
    if (!ew)
        return;

    ew->setEmbeddingApi(embeddingApi, this);
    connect(ew, &EmbeddingWorker::statusChanged, this, &VectorIndexDBus::IndexStatus);
    connect(ew, &EmbeddingWorker::indexDeleted, this, &VectorIndexDBus::IndexDeleted);
}

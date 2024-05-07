// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "vectorindexdbus.h"

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

VectorIndexDBus::VectorIndexDBus(QObject *parent) : QObject(parent)
{
    init();
}

VectorIndexDBus::~VectorIndexDBus()
{
    if (ew)
        delete ew;
    ew = nullptr;
}

bool VectorIndexDBus::Create(const QStringList &files, const QString &key)
{
    qInfo() << "Index Create!";
    return ew->doCreateIndex(files, key);
}

bool VectorIndexDBus::Delete(const QStringList &files, const QString &key)
{
    qInfo() << "Index Delete!";
    return ew->doDeleteIndex(files, key);
}

bool VectorIndexDBus::Enable()
{
    return (bgeModel->isRunning()) || (ModelhubWrapper::isModelhubInstalled() &&
        ModelhubWrapper::isModelInstalled(dependModel()));
}

QStringList VectorIndexDBus::Search(const QString &query, const QString &key, int topK)
{
    qInfo() << "Index Search!";
    return  ew->doVectorSearch(query, key, topK);
}

QJsonObject VectorIndexDBus::embeddingApi(const QStringList &texts, bool isQuery, void *user)
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
    data["is_query"] = isQuery;
    data["query_texts"] = jsonValue;
    //data["input"] = jsonValue;

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
    ew = new EmbeddingWorker(this);
    ew->setEmbeddingApi(embeddingApi, this);

    if (ModelhubWrapper::isModelhubInstalled()) {
        if (!ModelhubWrapper::isModelInstalled(dependModel()))
            qWarning() << QString("VectorIndex needs model %0, but it is not avalilable").arg(dependModel());
    } else {
        qWarning() << "VectorIndex depends on deepin modehub, but it is not avalilable";
    }

    bgeModel = new ModelhubWrapper(dependModel(), this);
}

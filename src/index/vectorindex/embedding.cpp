// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "embedding.h"
#include "vectorindex.h"
#include "database/embeddatabase.h"
#include "../global_define.h"

#include <QRegularExpression>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDebug>
#include <QDir>
#include <QEventLoop>
#include <QtConcurrent/QtConcurrent>

#include <QNetworkAccessManager>
#include <QNetworkReply>

#include <docparser.h>

Embedding::Embedding(QObject *parent)
    : QObject(parent)
{
    //数据库所有数据信息
    //updateDataInfo();
    init();
}

void Embedding::embeddingDocument(const QString &docFilePath, const QString &key, bool isOverWrite)
{
    QFileInfo docFile(docFilePath);
    if (!docFile.exists()) {
        qWarning() << docFilePath << "not exist";
        return;
    }

    if (isDupDocument(key, docFilePath)) {
        qWarning() << docFilePath << "dup";
        return;
    }
//    QJsonObject docDataObj;
//    docDataObj = getDataInfo(key);
//    for (const QString &it : docDataObj.keys()) {
//        QJsonObject data = docDataObj[it].toObject();
//        if (data["docFilePath"].toString() == docFilePath) {
//            qWarning() << docFilePath << "dup";
//            return;
//        }
//    }

    QString contents = DocParser::convertFile(docFilePath.toStdString()).c_str();    
    if (contents.isEmpty())
        return;
    qInfo() << "embedding " << docFilePath;

    //文本分块
    QStringList chunks;
    chunks = textsSpliter(contents);

    //元数据、文本存储
    int continueID = getDBLastID(key);  //datainfo 起始ID
    qInfo() << "-------------" << continueID;
//    if (!isOverWrite) {
//        //不覆盖重写 ID接着之前的
//        //读取DataInfo
//        //和索引文件等，
//        //都存在工作目录/embedding/+key的目录下。
//        if (!docDataObj.isEmpty())
//            continueID = docDataObj.keys().count();
//    }
    QStringList insertList;
    for (int i = 0; i < chunks.count(); i++) {
        if (chunks[i].isEmpty())
            continue;
        //dataInfo.json 存储
//        QJsonObject docDataInfo;
//        docDataInfo["docFilePath"] = docFilePath;
//        docDataInfo["text"] = chunks[i];
//        docDataObj[QString::number(continueID)] = docDataInfo;

        QString queryStr = "INSERT INTO embedding_metadata (id, source, content) VALUES ("
                + QString::number(continueID) + ", '" + docFilePath + "', " + "'" + chunks[i] + "')";
        insertList << queryStr;
//        QString queryStr = "INSERT INTO %1 (%2, %3, %4) "
//                           "VALUES (%5, %6, %7)";
//        queryStr = queryStr.arg(kEmbeddingDBMetaDataTable).arg(kEmbeddingDBMetaDataTableID).
//                arg(kEmbeddingDBMetaDataTableSource).arg(kEmbeddingDBMetaDataTableContent).
//                arg(continueID).arg(docFilePath).arg(chunks[i]);

        //IDS添加
        embeddingIds << continueID;
        continueID += 1;
    }
    //批量插入metaData到数据库
    //[id、source、content]
    //组装SQL语句 列表 插入
    if (!batchInsertDataToDB(insertList, key)) {
        qWarning() << docFilePath << "Insert DB failed.";
    }
    //向量化文本块，生成向量vector
    embeddingTexts(chunks);
    return;
}

void Embedding::embeddingTexts(const QStringList &texts)
{
    if (texts.isEmpty())
        return;

    QJsonObject emdObject;
    emdObject = onHttpEmbedding(texts, false);

//    QJsonArray embeddingsArray = emdObject["embedding"].toArray();
//    for(auto embeddingObject : embeddingsArray) {
//        QJsonArray vectorArray = embeddingObject.toObject()["embedding"].toArray();
//        for (auto value : vectorArray) {
//            embeddingVector << static_cast<float>(value.toDouble());
//        }
//    }

    QJsonArray embeddingsArray = emdObject["embedding"].toArray();
    for(auto embeddingObject : embeddingsArray) {
        QJsonArray vectorArray = embeddingObject.toArray();
        for (auto value : vectorArray) {
            embeddingVector << static_cast<float>(value.toDouble());
        }
    }
}

void Embedding::embeddingQuery(const QString &query, QVector<float> &queryVector)
{
    /* query:查询问题
     * queryVector:问题向量化
     *
     * 调用接口将query进行向量化，结果通过queryVector传递float指针
    */

    QStringList queryTexts;
    queryTexts << "为这个句子生成表示以用于检索相关文章:" + query;
    QJsonObject emdObject;
    emdObject = onHttpEmbedding(queryTexts, true);

    //获取query
    //local
//    QJsonArray embeddingsArray = emdObject["embedding"].toArray();
//    for(auto embeddingObject : embeddingsArray) {
//        QJsonArray vectorArray = embeddingObject.toObject()["embedding"].toArray();
//        for (auto value : vectorArray) {
//            queryVector << static_cast<float>(value.toDouble());
//        }
//    }

    QJsonArray embeddingsArray = emdObject["embedding"].toArray();
    for (auto value : embeddingsArray) {
        queryVector << static_cast<float>(value.toDouble());
    }
}

bool Embedding::clearAllDBTable(const QString &key)
{
    //重置Table [embedding_metadata]
    QList<QVariantMap> result;
    bool ok = QtConcurrent::run([key, &result](){
        QString query = "DELETE FROM " + QString(kEmbeddingDBMetaDataTable);
        return EmbedDBManagerIns->executeQuery(key + ".db", query, result);
    });
    return ok;
}

bool Embedding::batchInsertDataToDB(const QStringList &inserQuery, const QString &key)
{
    bool ok;
    if (inserQuery.isEmpty())
        return false;
    QFuture<void> future = QtConcurrent::run([key, &ok, inserQuery](){
        ok = EmbedDBManagerIns->commitTransaction(key + ".db", inserQuery);
    });
    future.waitForFinished();
    return ok;
}

int Embedding::getDBLastID(const QString &key)
{
    QList<QVariantMap> result;
    QFuture<void> future =  QtConcurrent::run([key, &result](){
        QString query = "SELECT id FROM " + QString(kEmbeddingDBMetaDataTable) + " ORDER BY id DESC LIMIT 1";
        return EmbedDBManagerIns->executeQuery(key + ".db", query, result);
    });

    future.waitForFinished();
    if (result.isEmpty() || !result[0]["id"].isValid())
        return 0;
    return result[0]["id"].toInt() + 1;
}

void Embedding::createEmbedDataTable(const QString &key)
{
    QFuture<void> future =  QtConcurrent::run([key](){
        QString createTableSQL = "CREATE TABLE IF NOT EXISTS " + QString(kEmbeddingDBMetaDataTable) + " (id INTEGER PRIMARY KEY, source TEXT, content TEXT)";
        return EmbedDBManagerIns->executeQuery(key + ".db", createTableSQL);
    });
    return future.waitForFinished();
}

bool Embedding::isDupDocument(const QString &key, const QString &docFilePath)
{
    QList<QVariantMap> result;
    QFuture<void> future =  QtConcurrent::run([key, docFilePath, &result](){
        QString query = "SELECT CASE WHEN EXISTS (SELECT 1 FROM " + QString(kEmbeddingDBMetaDataTable)
                + " WHERE source = '" + docFilePath + "') THEN 1 ELSE 0 END";
        return EmbedDBManagerIns->executeQuery(key + ".db", query, result);
    });

    future.waitForFinished();
    if (result.isEmpty() || !result[0]["id"].isValid())
        return 0;
    return result[0]["id"].toBool();
}

void Embedding::embeddingClear()
{
    //isStop = true;
    embeddingVector.clear();
    embeddingIds.clear();
}

QVector<float> Embedding::getEmbeddingVector()
{

    return embeddingVector;
}

QVector<faiss::idx_t> Embedding::getEmbeddingIds()
{
    return embeddingIds;
}

void Embedding::init()
{
    //connect(this, &Embedding::embeddinged, vectorIndex, &VectorIndex::onEmbeddinged);
}

QStringList Embedding::textsSpliter(const QString &texts)
{
    QStringList chunks;
    QStringList splitTexts;

    QRegularExpression regex("[.。\\n]");
    //QRegularExpression regex("END");
    splitTexts = texts.split(regex, QString::SplitBehavior::SkipEmptyParts);

    int chunkSize = 0;
    QString chunk = "";
    for (const QString &text : splitTexts) {
        chunkSize += text.size();
        if (chunkSize > kMaxChunksSize) {
            if (!chunk.isEmpty())
                chunks << chunk;
            chunk = text;
            chunkSize = 0;
        } else {
            chunk += text;
        }
    }
    if (!chunk.isEmpty())
        chunks << chunk;

    return chunks;
}

QJsonObject Embedding::getDataInfo(const QString &key)
{
    QFile dataInfoFile(workerDir() + QDir::separator() + key + kDataInfo);
    QDir dir(QFileInfo(dataInfoFile).absolutePath());
    if (!dir.exists()) {
        if (!dir.mkpath(dir.absolutePath())) {
            qWarning() << dir.absolutePath() << " directory isn't exists!";
            return {};
        }
    }

    if (!dataInfoFile.open(QIODevice::ReadOnly)) {
        qDebug() << "Failed to open data info file.";
        return {};
    }

    QByteArray jsonData = dataInfoFile.readAll();
    QJsonDocument jsonDoc = QJsonDocument::fromJson(jsonData);

    if (jsonDoc.isNull()) {
        qDebug() << "Failed to parse JSON file.";
        return {};
    }

    QJsonObject dataInfoJson = jsonDoc.object();

    return dataInfoJson;
}

void Embedding::updateDataInfo(const QJsonObject &dataInfos, const QString &key)
{
    if (dataInfos.isEmpty())
        return;

    QFile dataInfoFile(workerDir() + QDir::separator() + key + kDataInfo);
    QDir dir(QFileInfo(dataInfoFile).absolutePath());
    if (!dir.exists()) {
        if (!dir.mkpath(dir.absolutePath())) {
            qWarning() << dir.absolutePath() << " directory isn't exists!";
            return;
        }
    }
    if (dataInfoFile.open(QIODevice::WriteOnly)) {
        QJsonDocument jsonDoc(dataInfos);
        dataInfoFile.write(jsonDoc.toJson());
        dataInfoFile.close();
        qDebug() << "Data info JSON file updated successfully.";
    } else {
        qDebug() << "Failed to save data info JSON file.";
    }
}

QJsonObject Embedding::onHttpEmbedding(const QStringList &texts, bool isQuery)
{
    QNetworkAccessManager manager;
    QNetworkRequest request(QUrl("http://10.8.11.110:8000"));
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

QStringList Embedding::loadTextsFromIndex(const QVector<faiss::idx_t> &ids, const QString &indexKey)
{
    QStringList docFilePaths;
    QStringList texts;

    QString idStr = "(";
    for (faiss::idx_t id : ids) {
        if (ids.last() == id) {
            idStr += QString::number(id) + ")";
            break;
        }
        idStr += QString::number(id) + ", ";
    }

    //TODO: SQL 查询结果只满足TOPK，没有按相似度排序
    QList<QVariantMap> result;
    QFuture<void> future =  QtConcurrent::run([indexKey, idStr, &result](){
        QString query = "SELECT * FROM " + QString(kEmbeddingDBMetaDataTable) + " WHERE id IN " + idStr;
        return EmbedDBManagerIns->executeQuery(indexKey + ".db", query, result);
    });

    future.waitForFinished();
    if (result.isEmpty())
        return {};

    for (const QVariantMap &res : result) {
        texts << res["content"].toString();
        docFilePaths << res["source"].toString();
    }
    return texts;
}

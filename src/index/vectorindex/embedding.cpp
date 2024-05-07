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
#include <QtConcurrent/QtConcurrent>

#include <docparser.h>

Embedding::Embedding(QObject *parent)
    : QObject(parent)
{
    //数据库所有数据信息
    //updateDataInfo();
    init();
}

void Embedding::embeddingDocument(const QString &docFilePath, const QString &key)
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

    QStringList insertList;
    for (int i = 0; i < chunks.count(); i++) {
        if (chunks[i].isEmpty())
            continue;

        QString queryStr = "INSERT INTO embedding_metadata (id, source, content) VALUES ("
                + QString::number(continueID) + ", '" + docFilePath + "', " + "'" + chunks[i] + "')";
        insertList << queryStr;

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
    emdObject = onHttpEmbedding(texts, apiData);

    QJsonArray embeddingsArray = emdObject["embedding"].toArray();
    for(auto embeddingObject : embeddingsArray) {
        QJsonArray vectorArray = embeddingObject.toObject()["embedding"].toArray();
        for (auto value : vectorArray) {
            embeddingVector << static_cast<float>(value.toDouble());
        }
    }

//    QJsonArray embeddingsArray = emdObject["embedding"].toArray();
//    for(auto embeddingObject : embeddingsArray) {
//        QJsonArray vectorArray = embeddingObject.toArray();
//        for (auto value : vectorArray) {
//            embeddingVector << static_cast<float>(value.toDouble());
//        }
//    }
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
    emdObject = onHttpEmbedding(queryTexts, apiData);

    //获取query
    //local
    QJsonArray embeddingsArray = emdObject["embedding"].toArray();
    for(auto embeddingObject : embeddingsArray) {
        QJsonArray vectorArray = embeddingObject.toObject()["embedding"].toArray();
        for (auto value : vectorArray) {
            queryVector << static_cast<float>(value.toDouble());
        }
    }

//    QJsonArray embeddingsArray = emdObject["embedding"].toArray();
//    for (auto value : embeddingsArray) {
//        queryVector << static_cast<float>(value.toDouble());
//    }
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

QStringList Embedding::textsSpliter(QString &texts)
{
    QStringList chunks;
    QStringList splitTexts;

    QString splitPattern = "。";
    texts.replace("\n", "");
    QRegularExpression regex(splitPattern);
    //QRegularExpression regex("END");
    splitTexts = texts.split(regex, QString::SplitBehavior::SkipEmptyParts);

    int chunkSize = 0;
    QString chunk = "";
    for (auto text : splitTexts) {
        text += splitPattern;
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

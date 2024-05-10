// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "private/embeddingworker_p.h"
#include "embeddingworker.h"
#include "config/configmanager.h"
#include "database/embeddatabase.h"
#include "global_define.h"

#include <QDebug>
#include <QDir>

#include <index/private/embeddingworker_p.h>

EmbeddingWorkerPrivate::EmbeddingWorkerPrivate(QObject *parent)
    : QObject(parent)
{
    init();
}

void EmbeddingWorkerPrivate::init()
{
    //初始化 向量化 工作目录
    QDir dir;
    if (!dir.exists(workerDir())) {
        if (!dir.mkpath(workerDir())) {
            qWarning() << "Unable to create directory: " << workerDir();
            return;
        }
    }

    //embedding、向量索引
    embedder = new Embedding(this);
    indexer = new VectorIndex(this);
}

bool EmbeddingWorkerPrivate::enableEmbedding(const QString &file)
{
    //TODO：读取不到内容！
    //const auto &blacklist = ConfigManagerIns->value(ENABLE_EMBEDDING_FILES_LIST_GROUP, ENABLE_EMBEDDING_PATHS, QStringList()).toStringList();

    //定义可embedding的文件目录
    QStringList blacklist;
    blacklist << "~/Documents/Embedding";

    return std::any_of(blacklist.begin(), blacklist.end(), [&file](const QString &path) {
        return !file.startsWith(path);
    });
}

QStringList EmbeddingWorkerPrivate::embeddingPaths()
{
    QStringList embeddingFilePaths;
    QString embeddingPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/Embedding";
    QDir dir(embeddingPath);
    if (!dir.exists()) {
        qDebug() << "Embedding directory does not exist:" << embeddingPath;
        return {};
    }

    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot);
    QFileInfoList embeddingList = dir.entryInfoList();

    foreach (const QFileInfo &fileInfo, embeddingList) {
        embeddingFilePaths << fileInfo.absoluteFilePath();
    }

    return embeddingFilePaths;
}

bool EmbeddingWorkerPrivate::createSystemAssistantIndex(const QString &indexKey)
{
    /* 创建系统助手索引
     * 所有 ~/Documents/Embedding 文件夹下所有文档的索引
     *
    * 1.索引是否存在？ 通过dataINfo.json中的文件名判断。存在则跳过。XXX
    * 2.数量较少，暂时先用Flat构建成一个索引。
    *
    */
       //强制更新~/Documents/Embedding/下的所有文件的向量索引
       //dataINfo\embeddingInfo 都会进行覆盖
       //embeddingPaths:系统助手文档文件夹
    //创建所有、重置表
    embedder->createEmbedDataTable(indexKey);
    embedder->clearAllDBTable(indexKey);

    QStringList embeddingFilePaths = embeddingPaths();
    for (const QString &embeddingfile : embeddingFilePaths)
        embedder->embeddingDocument(embeddingfile, indexKey);

    bool createResult = indexer->createIndex(EmbeddingDim, embedder->getEmbeddingVector(), embedder->getEmbeddingIds(), kSystemAssistantKey);
    return createResult;
}

bool EmbeddingWorkerPrivate::createAllIndex(const QStringList &files, const QString &indexKey)
{
    QString indexDirStr = workerDir() + QDir::separator() + indexKey;
    QDir dir(indexDirStr);

    //创建索引目录
    if (!dir.mkpath(indexDirStr)) {
        qWarning() << indexKey << " directory can't create!";
        return false;
    }

    //创建所有、重置表
    embedder->createEmbedDataTable(indexKey);
    embedder->clearAllDBTable(indexKey);

    for (const QString &embeddingfile : files)
        embedder->embeddingDocument(embeddingfile, indexKey);

    bool createResult = indexer->createIndex(EmbeddingDim, embedder->getEmbeddingVector(), embedder->getEmbeddingIds(), indexKey);
    return createResult;
}

bool EmbeddingWorkerPrivate::updateIndex(const QStringList &files, const QString &indexKey)
{
    for (const QString &embeddingfile : files) {
        embedder->embeddingDocument(embeddingfile, indexKey);
    }

    bool updateResult = indexer->updateIndex(EmbeddingDim, embedder->getEmbeddingVector(), embedder->getEmbeddingIds(), indexKey);
    return updateResult;
}

bool EmbeddingWorkerPrivate::deleteIndex(const QStringList &files, const QString &indexKey)
{
    QString sourceStr = "(";
    for (const QString &source : files) {
        if (files.last() == source) {
            sourceStr += "'" + source + "')";
            break;
        }
        sourceStr += "'" + source + "', ";
    }

    QList<QVariantMap> result;
    QFuture<void> future =  QtConcurrent::run([indexKey, sourceStr, &result](){
        QString queryDeleteID = "SELECT id FROM " + QString(kEmbeddingDBMetaDataTable) + " WHERE source IN " + sourceStr;
        QString queryDelete = "DELETE FROM " + QString(kEmbeddingDBMetaDataTable) + " WHERE source IN " + sourceStr;
        EmbedDBManagerIns->executeQuery(indexKey + ".db", queryDeleteID, result);
        EmbedDBManagerIns->executeQuery(indexKey + ".db", queryDelete);
    });

    future.waitForFinished();
    if (result.isEmpty())
        return false;

    QVector<faiss::idx_t> deleteID;
    for (const QVariantMap &res : result) {
        if (res["id"].isValid())
            deleteID << res["id"].toInt();
    }

    return indexer->deleteIndex(indexKey, deleteID);
}

QStringList EmbeddingWorkerPrivate::vectorSearch(const QString &query, const QString &key, int topK)
{
    QVector<float> queryVector;  //查询向量 传递float指针
    embedder->embeddingQuery(query, queryVector);

    QVector<faiss::idx_t> I1 = indexer->vectorSearch(topK, queryVector.data(), key);
    qInfo() << I1;
    return embedder->loadTextsFromIndex(I1, key);
}

bool EmbeddingWorkerPrivate::isIndexExists(const QString &indexKey)
{
    //直接根据索引的key目录是否存在，判断index是否已经创建
    QString indexDirStr = indexDir(indexKey);
    QDir dir(indexDirStr);
    return dir.exists();
}

QString EmbeddingWorkerPrivate::indexDir(const QString &indexKey)
{
    return workerDir() + QDir::separator() + indexKey;
}

QStringList EmbeddingWorkerPrivate::getIndexDocs(const QString &indexKey)
{
    QList<QVariantMap> result;
    QFuture<void> future =  QtConcurrent::run([indexKey, &result](){
        QString queryDocs = "SELECT source FROM " + QString(kEmbeddingDBMetaDataTable);
        EmbedDBManagerIns->executeQuery(indexKey + ".db", queryDocs, result);
    });

    future.waitForFinished();
    if (result.isEmpty())
        return {};

    QStringList queryResult;
    for (const QVariantMap &res : result) {
        if (res["id"].isValid())
            queryResult << res["id"].toString();
    }
    QSet<QString> stringSet;
    foreach (const QString &str, queryResult) {
        stringSet.insert(str);
    }
    QStringList uniqueList = stringSet.toList();
    return uniqueList;
}

EmbeddingWorker::EmbeddingWorker(QObject *parent)
    : QObject(parent),
      d(new EmbeddingWorkerPrivate(this))
{
    //索引建立成功，完成后续操作
    connect(this, &EmbeddingWorker::indexCreateSuccess, d->embedder, &Embedding::onIndexCreateSuccess);
}

EmbeddingWorker::~EmbeddingWorker()
{
    if (d->embedder) {
        delete d->embedder;
        d->embedder = nullptr;
    }

    if (d->indexer) {
        delete d->indexer;
        d->indexer = nullptr;
    }
}

void EmbeddingWorker::setEmbeddingApi(embeddingApi api, void *user)
{
    d->embedder->setEmbeddingApi(api, user);
}

void EmbeddingWorker::onFileCreated(const QString &file)
{
    qInfo() << "--------------file create------------" << file;
    if (d->enableEmbedding(file)) {
        return;
    }
}

void EmbeddingWorker::onFileDeleted(const QString &file)
{
    qInfo() << "--------------file delete--------------" << file;
}

/*            [key] 知识库标识
 * 如SystemAssistant为系统助手知识库的标识
 * 索引创建、向量检索时都需此参数作为.faiss索引文件的名称
 * 后续区分知识库，可用此参数。
 */
bool EmbeddingWorker::doCreateIndex(const QStringList &files, const QString &key)
{
    if (files.isEmpty() && key != kSystemAssistantKey)
        return false;

    if (key == kSystemAssistantKey) {
        qInfo() << "create system Assistant index.";
        //创建系统助手索引
        QFuture<bool> future = QtConcurrent::run([key, this](){
            return d->createSystemAssistantIndex(key);
        });
        QFutureWatcher<bool> *futureWatcher = new QFutureWatcher<bool>(this);
        QObject::connect(futureWatcher, &QFutureWatcher<QString>::finished, [files, futureWatcher, key, this](){
            if (futureWatcher->future().result()) {
                Q_EMIT statusChanged(files, IndexCreateStatus::Success, key);
                Q_EMIT indexCreateSuccess(key);
                qInfo() << "System index create Success";
            } else {
                Q_EMIT statusChanged(files, IndexCreateStatus::Failed, key);
                qInfo() << "System index create Failed";
            }
            futureWatcher->deleteLater();
        });
        futureWatcher->setFuture(future);
        return true;
    }

    /* 创建用户知识库向量索引 [Key]
     * Files List全部创建
     * 索引、数据文件全部保存在key目录下
     * 索引、数据文件全部根据所提供的文档更新一遍
     */
    if (d->isIndexExists(key)) {
        qInfo() << "Index Key exists!";        
        return false; //索引存在 不可创建覆盖
    }

    QFuture<bool> future = QtConcurrent::run([key, files, this](){
        return d->createAllIndex(files, key);
    });
    QFutureWatcher<bool> *futureWatcher = new QFutureWatcher<bool>(this);
    QObject::connect(futureWatcher, &QFutureWatcher<QString>::finished, [files, futureWatcher, key, this](){
        if (futureWatcher->future().result()) {
            Q_EMIT statusChanged(files, IndexCreateStatus::Success, key);
            Q_EMIT indexCreateSuccess(key);
            qInfo() << "Index create Success";
        } else {
            Q_EMIT statusChanged(files, IndexCreateStatus::Failed, key);
            qInfo() << "Index create Failed";
        }
        futureWatcher->deleteLater();
    });
    futureWatcher->setFuture(future);
    return true;
}

bool EmbeddingWorker::doUpdateIndex(const QStringList &files, const QString &key)
{
    if (files.isEmpty() || !d->isIndexExists(key))
        return false;

    QFuture<bool> future = QtConcurrent::run([key, files, this](){
        return d->updateIndex(files, key);
    });
    QFutureWatcher<bool> *futureWatcher = new QFutureWatcher<bool>(this);
    QObject::connect(futureWatcher, &QFutureWatcher<QString>::finished, [files, futureWatcher, key, this](){
        if (futureWatcher->future().result()) {
            Q_EMIT statusChanged(files, IndexCreateStatus::Success, key);
            Q_EMIT indexCreateSuccess(key);
            qInfo() << "Index update Success";
        } else {
            Q_EMIT statusChanged(files, IndexCreateStatus::Failed, key);
            qInfo() << "Index update Failed";
        }
        futureWatcher->deleteLater();
    });
    futureWatcher->setFuture(future);
    return true;
}

bool EmbeddingWorker::doDeleteIndex(const QStringList &files, const QString &key)
{
    if (files.isEmpty())
        return false;

     bool ret = d->deleteIndex(files, key);

     if (ret)
         Q_EMIT indexDeleted(files, key);

     return ret;
}

QStringList EmbeddingWorker::doVectorSearch(const QString &query, const QString &key, int topK)
{
    if (query.isEmpty()) {
        qWarning() << "query is empty!";
        return {};
    }
    return d->vectorSearch(query, key, topK);
}

bool EmbeddingWorker::indexExists(const QString &key)
{
    return d->isIndexExists(key);
}

QStringList EmbeddingWorker::getDocFile(const QString &key)
{
    return d->getIndexDocs(key);
}

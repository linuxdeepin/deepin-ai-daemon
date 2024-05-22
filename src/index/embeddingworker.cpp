// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "private/embeddingworker_p.h"
#include "embeddingworker.h"
#include "config/configmanager.h"
#include "database/embeddatabase.h"
#include "global_define.h"
#include "index/indexmanager.h"

#include <QDebug>
#include <QDir>
#include <QFileInfo>

#include <dirent.h>
#include <sys/stat.h>

EmbeddingWorkerPrivate::EmbeddingWorkerPrivate(QObject *parent)
    : QObject(parent)
{
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
    embedder = new Embedding(&dataBase, &dbMtx, appID, this);
    indexer = new VectorIndex(&dataBase, &dbMtx, appID, this);

    QString databasePath;
    if (appID == kSystemAssistantKey)
        databasePath = QString("%0.db").arg(kSystemAssistantData);
    else
        databasePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QDir::separator() +  appID + ".db";;
    dataBase = EmbedDBVendorIns->addDatabase(databasePath);

    if (appID == kUosAIAssistant) {
        // uos-ai 另存原文档
        m_saveAsDoc = true;
    }
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


bool EmbeddingWorkerPrivate::updateIndex(const QStringList &files)
{
    embedder->createEmbedDataTable();

    if (files.isEmpty())
        return false;

    bool embedRes = true;
    for (const QString &embeddingfile : files) {
        if (m_saveAsDoc)
            embedRes &= embedder->embeddingDocumentSaveAs(embeddingfile);
        else
            embedRes &= embedder->embeddingDocument(embeddingfile);
    }

    bool updateRes = false;
    if (embedRes)
        updateRes = indexer->updateIndex(EmbeddingDim, embedder->getEmbedVectorCache());

    return updateRes && embedRes;
}

bool EmbeddingWorkerPrivate::deleteIndex(const QStringList &files)
{
    QString sourceStr = "(";
    for (const QString &source : files) {
        if (files.last() == source) {
            sourceStr += "'" + source + "')";
            break;
        }
        sourceStr += "'" + source + "', ";
    }

    //删除缓存中的数据、重置缓存索引
    if (!embedder->getEmbedVectorCache().isEmpty()) {
        embedder->deleteCacheIndex(files);
        indexer->resetCacheIndex(EmbeddingDim, embedder->getEmbedVectorCache());
    }

    //删除已存储的数据
    QList<QVariantMap> result;
    QString queryDeleteID = "SELECT id FROM " + QString(kEmbeddingDBMetaDataTable) + " WHERE source IN " + sourceStr;
    QString queryDelete = "DELETE FROM " + QString(kEmbeddingDBMetaDataTable) + " WHERE source IN " + sourceStr;
    {
        QMutexLocker lk(&dbMtx);
        EmbedDBVendorIns->executeQuery(&dataBase, queryDeleteID, result);        
        EmbedDBVendorIns->executeQuery(&dataBase, queryDelete);
    }

    // 索引deleteBitSet置1
    QString idsStr = "(";
    for (const QVariantMap &res : result) {
        faiss::idx_t id = res["id"].toInt();
        if (result.last() == res) {
            idsStr += "'" + QString::number(id) + "')";
            break;
        }
        idsStr += "'" + QString::number(id) + "', ";
    }
    QString updateBitSet = "UPDATE " + QString(kEmbeddingDBIndexSegTable) + " SET " + QString(kEmbeddingDBSegIndexTableBitSet)
                           + " = '" + QString::number(1) + "' WHERE id IN " + idsStr;
    {
        QMutexLocker lk(&dbMtx);
        EmbedDBVendorIns->executeQuery(&dataBase, updateBitSet);
    }

    return true;
}

QString EmbeddingWorkerPrivate::vectorSearch(const QString &query, int topK)
{
    QVector<float> queryVector;  //查询向量 传递float指针
    embedder->embeddingQuery(query, queryVector);

    QMap<float, faiss::idx_t> cacheSearchRes;
    QMap<float, faiss::idx_t> dumpSearchRes;
    indexer->vectorSearch(topK, queryVector.data(), cacheSearchRes, dumpSearchRes);
    QString res = embedder->loadTextsFromSearch(topK, cacheSearchRes, dumpSearchRes);
    return res;
}

QString EmbeddingWorkerPrivate::indexDir()
{
    return workerDir() + QDir::separator() + appID;
}

QStringList EmbeddingWorkerPrivate::getIndexDocs()
{
    //cache docs
    QStringList cacheDocs;
    QMap<faiss::idx_t, QPair<QString, QString>> cacheData = embedder->getEmbedDataCache();
    for (faiss::idx_t id : cacheData.keys()) {
        if (!cacheDocs.contains(cacheData[id].first))
            cacheDocs << cacheData[id].first;
    }

    //dump docs
    QStringList dumpDocs;
    QList<QVariantMap> result;
    {
        QMutexLocker lk(&dbMtx);
        QString queryDocs = "SELECT source FROM " + QString(kEmbeddingDBMetaDataTable);
        EmbedDBVendorIns->executeQuery(&dataBase, queryDocs, result);
    }
    QStringList queryResult;
    for (const QVariantMap &res : result) {
        if (res["id"].isValid())
            queryResult << res["id"].toString();
    }
    QSet<QString> stringSet;
    foreach (const QString &str, queryResult) {
        stringSet.insert(str);
    }
    dumpDocs = stringSet.toList();
    return cacheDocs + dumpDocs;
}

bool EmbeddingWorkerPrivate::isSupportDoc(const QString &file)
{
    QFileInfo fileInfo(file);
    QStringList suffixs;
    suffixs << "txt"
           << "doc"
           << "docx"
           << "xls"
           << "xlsx"
           << "ppt"
           << "pptx"
           << "pdf";

    return suffixs.contains(fileInfo.suffix());
}

bool EmbeddingWorkerPrivate::isFilter(const QString &file)
{
    const auto &blacklist = ConfigManagerIns->value(BLACKLIST_GROUP, BLACKLIST_PATHS, QStringList()).toStringList();
    return std::any_of(blacklist.begin(), blacklist.end(), [&file](const QString &path) {
            return file.startsWith(path);
        });
}

EmbeddingWorker::EmbeddingWorker(const QString &appid, QObject *parent)
    : QObject(parent),
      d(new EmbeddingWorkerPrivate(this))
{
    Q_ASSERT(!appid.isEmpty());

    d->appID = appid;
    d->init();

    moveToThread(&d->workThread);
    d->workThread.start();

    connect(this, &EmbeddingWorker::stopEmbedding, this, &EmbeddingWorker::doIndexDump);
    connect(d->indexer, &VectorIndex::indexDump, this, &EmbeddingWorker::doIndexDump);

    //索引建立成功，完成后续操作
    //connect(this, &EmbeddingWorker::indexCreateSuccess, d->embedder, &Embedding::onIndexCreateSuccess);
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

    //TODO:停止embeddding、已建索引落盘、数据存储等
}

void EmbeddingWorker::setEmbeddingApi(embeddingApi api, void *user)
{
    d->embedder->setEmbeddingApi(api, user);
}

void EmbeddingWorker::stop()
{
    d->m_creatingAll = false;
    Q_EMIT stopEmbedding();
}

void EmbeddingWorker::saveAllIndex()
{
    Q_EMIT stopEmbedding();
}

EmbeddingWorker::IndexCreateStatus EmbeddingWorker::createAllState()
{
    return  d->m_creatingAll ? Creating : Success;
}

void EmbeddingWorker::setWatch(bool watch)
{
    auto idx = IndexManager::instance();
    Q_ASSERT(idx);
    disconnect(idx, nullptr, this, nullptr);
    if (watch) {
        connect(idx, &IndexManager::fileCreated, this, &EmbeddingWorker::onFileMonitorCreate);
        connect(idx, &IndexManager::fileDeleted, this, &EmbeddingWorker::onFileMonitorDelete);
        //connect(idx, &IndexManager::fileAttributeChanged, this, );
    }
}

bool EmbeddingWorker::doCreateIndex(const QStringList &files)
{
    //过滤文档
    for (auto it : files) {
        if (!d->isSupportDoc(it)) {
            qDebug() << it << " doc not support!";
            return false;
        }
    }

    bool ret = d->updateIndex(files);
    if (!ret) {
        Q_EMIT statusChanged(d->appID, files, Failed);
        qWarning() << "Index update Failed";
    } else {
        Q_EMIT statusChanged(d->appID, files, Success);
    }

    return ret;
}

bool EmbeddingWorker::doDeleteIndex(const QStringList &files)
{
    for (auto it : files) {
        if (!d->isSupportDoc(it)) {
            qDebug() << it << " doc not support!" << it;
            return false;
        }
    }

    bool ret = d->deleteIndex(files);
    if (!ret)
        qWarning() << "Index Delete Failed";
    else
        Q_EMIT indexDeleted(d->appID, files);

    return ret;
}

void EmbeddingWorker::onFileMonitorCreate(const QString &file)
{
    doCreateIndex(QStringList(file));
}

void EmbeddingWorker::onFileMonitorDelete(const QString &file)
{
    doDeleteIndex(QStringList(file));
}

void EmbeddingWorker::doIndexDump()
{
    d->embedder->doIndexDump();
    d->indexer->doIndexDump();
}

void EmbeddingWorker::onCreateAllIndex()
{
    d->m_creatingAll = true;
    QString path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    traverseAndCreate(path);
    d->m_creatingAll = false;
}

void EmbeddingWorker::traverseAndCreate(const QString &path)
{
    if (!d->m_creatingAll || d->isFilter(path))
        return;

    const std::string tmp = path.toStdString();
    const char *filePath = tmp.c_str();
    struct stat st;
    if (stat(filePath, &st) != 0)
        return;

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = nullptr;
        if (!(dir = opendir(filePath))) {
            qWarning() << "can not open: " << filePath;
            return;
        }

        struct dirent *dent = nullptr;
        char fn[FILENAME_MAX] = { 0 };
        strcpy(fn, filePath);
        size_t len = strlen(filePath);
        if (strcmp(filePath, "/"))
            fn[len++] = '/';

        // traverse
        while ((dent = readdir(dir)) && d->m_creatingAll) {
            if (dent->d_name[0] == '.')
                continue;

            if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
                continue;

            strncpy(fn + len, dent->d_name, FILENAME_MAX - len);
            traverseAndCreate(fn);
        }

        if (dir)
            closedir(dir);
        return;
    }

//    if (isCheck && !checkUpdate(writer->getReader(), file, type))
//        return;


    if (!d->isSupportDoc(path))
        return;

    if (d->m_creatingAll)
        d->updateIndex({path});
}

QString EmbeddingWorker::doVectorSearch(const QString &query, int topK)
{
    if (query.isEmpty()) {
        qWarning() << "query is empty!";
        return {};
    }
    return d->vectorSearch(query,topK);
}

QStringList EmbeddingWorker::getDocFile()
{
    return d->getIndexDocs();
}

// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "indexworker.h"
#include "private/indexworker_p.h"
#include "parser/audiopropertyparser.h"
#include "parser/videopropertyparser.h"
#include "parser/imagepropertyparser.h"
#include "config/configmanager.h"

#include "analyzer/chineseanalyzer.h"

#include <QStandardPaths>
#include <QRegularExpression>
#include <QMimeDatabase>
#include <QMetaEnum>
#include <QDateTime>
#include <QTimer>
#include <QDebug>
#include <QDir>

#include <dirent.h>
#include <malloc.h>

using namespace Lucene;

IndexWorkerPrivate::IndexWorkerPrivate(QObject *parent)
    : QObject(parent)
{
    propertyParsers.insert("default", new AbstractPropertyParser(this));
    propertyParsers.insert("image/*", new ImagePropertyParser(this));
    propertyParsers.insert("audio/*", new AudioPropertyParser(this));
    propertyParsers.insert("video/*", new VideoPropertyParser(this));
}

bool IndexWorkerPrivate::indexExists()
{
    return IndexReader::indexExists(FSDirectory::open(indexStoragePath().toStdWString()));
}

bool IndexWorkerPrivate::isFilter(const QString &file)
{
    const auto &blacklist = ConfigManagerIns->value(BLACKLIST_GROUP, BLACKLIST_PATHS, QStringList()).toStringList();
    return std::any_of(blacklist.begin(), blacklist.end(), [&file](const QString &path) {
        return file.startsWith(path);
    });
}

Lucene::IndexWriterPtr IndexWorkerPrivate::newIndexWriter(bool create)
{
    return newLucene<IndexWriter>(FSDirectory::open(indexStoragePath().toStdWString()),
                                  newLucene<ChineseAnalyzer>(),
                                  create,
                                  IndexWriter::MaxFieldLengthLIMITED);
}

Lucene::IndexReaderPtr IndexWorkerPrivate::newIndexReader()
{
    return IndexReader::open(FSDirectory::open(indexStoragePath().toStdWString()), true);
}

void IndexWorkerPrivate::doIndexTask(const Lucene::IndexWriterPtr &writer, const QString &file, IndexWorkerPrivate::IndexType type, bool isCheck)
{
    if (isStoped || isFilter(file))
        return;

    // limit file name length and level
    if (file.size() > FILENAME_MAX - 1 || file.count('/') > 20)
        return;

    const std::string tmp = file.toStdString();
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
        while ((dent = readdir(dir)) && !isStoped) {
            if (dent->d_name[0] == '.')
                continue;

            if (!strcmp(dent->d_name, ".") || !strcmp(dent->d_name, ".."))
                continue;

            strncpy(fn + len, dent->d_name, FILENAME_MAX - len);
            doIndexTask(writer, fn, type, isCheck);
        }

        if (dir)
            closedir(dir);
        return;
    }

    if (isCheck && !checkUpdate(writer->getReader(), file, type))
        return;
    indexFile(writer, file, type);
}

void IndexWorkerPrivate::indexFile(Lucene::IndexWriterPtr writer, const QString &file, IndexWorkerPrivate::IndexType type)
{
    Q_ASSERT(writer);
    indexFileCount++;
    try {
        switch (type) {
        case CreateIndex: {
            qDebug() << "Adding [" << file << "]";
            // 添加
            writer->addDocument(indexDocument(file));
            break;
        }
        case UpdateIndex: {
            qDebug() << "Update file: [" << file << "]";
            // 定义一个更新条件
            TermPtr term = newLucene<Term>(L"path", file.toStdWString());
            // 更新
            writer->updateDocument(term, indexDocument(file));
            break;
        }
        }

    } catch (const LuceneException &e) {
        QMetaEnum enumType = QMetaEnum::fromType<IndexWorkerPrivate::IndexType>();
        qWarning() << QString::fromStdWString(e.getError()) << " type: " << enumType.valueToKey(type);
    } catch (const std::exception &e) {
        QMetaEnum enumType = QMetaEnum::fromType<IndexWorkerPrivate::IndexType>();
        qWarning() << QString(e.what()) << " type: " << enumType.valueToKey(type);
    } catch (...) {
        qWarning() << "Index document failed! " << file;
    }
}

bool IndexWorkerPrivate::checkUpdate(const IndexReaderPtr &reader, const QString &file, IndexWorkerPrivate::IndexType &type)
{
    Q_ASSERT(reader);

    try {
        SearcherPtr searcher = newLucene<IndexSearcher>(reader);
        TermQueryPtr query = newLucene<TermQuery>(newLucene<Term>(L"path", file.toStdWString()));

        // 文件路径为唯一值，所以搜索一个结果就行了
        TopDocsPtr topDocs = searcher->search(query, 1);
        int32_t numTotalHits = topDocs->totalHits;
        if (numTotalHits == 0) {
            type = CreateIndex;
            return true;
        } else {
            DocumentPtr doc = searcher->doc(topDocs->scoreDocs[0]->doc);
            QFileInfo info(file);
            QString lastModified = info.lastModified().toString("yyyyMMddHHmmss");
            String storeTime = doc->get(L"lastModified");

            if (lastModified.toStdWString() != storeTime) {
                type = UpdateIndex;
                return true;
            }
        }
    } catch (const LuceneException &e) {
        qWarning() << QString::fromStdWString(e.getError()) << " file: " << file;
    } catch (const std::exception &e) {
        qWarning() << QString(e.what()) << " file: " << file;
    } catch (...) {
        qWarning() << "The file checked failed!" << file;
    }

    return false;
}

Lucene::DocumentPtr IndexWorkerPrivate::indexDocument(const QString &file)
{
    DocumentPtr doc = newLucene<Document>();
    const auto &properties = fileProperties(file);
    for (const auto &iter : properties) {
        doc->add(newLucene<Field>(iter.field.toStdWString(),
                                  iter.contents.toStdWString(),
                                  Field::STORE_YES,
                                  iter.analyzed ? Field::INDEX_ANALYZED : Field::INDEX_NOT_ANALYZED));
    }

    return doc;
}

QList<AbstractPropertyParser::Property> IndexWorkerPrivate::fileProperties(const QString &file)
{
    static QMimeDatabase database;

    QList<AbstractPropertyParser::Property> properties;
    const auto &type = database.mimeTypeForFile(file);
    const auto &mimeName = type.name();
    for (const auto &mimeRegx : propertyParsers.keys()) {
        QRegularExpression regx(mimeRegx);
        if (mimeName.contains(regx)) {
            properties = propertyParsers.value(mimeRegx)->properties(file);
            break;
        }
    }

    if (properties.isEmpty())
        properties = propertyParsers.value("default")->properties(file);

    return properties;
}

IndexWorker::IndexWorker(QObject *parent)
    : QObject(parent),
      d(new IndexWorkerPrivate(this))
{
}

void IndexWorker::stop()
{
    d->isStoped = true;
}

void IndexWorker::onFileAttributeChanged(const QString &file)
{
    if (d->isStoped)
        return;

    try {
        IndexWriterPtr writer = d->newIndexWriter();
        d->doIndexTask(writer, file, IndexWorkerPrivate::UpdateIndex);
        writer->optimize();
        writer->close();
    } catch (const LuceneException &e) {
        qWarning() << QString::fromStdWString(e.getError());
    } catch (const std::exception &e) {
        qWarning() << QString(e.what());
    } catch (...) {
        qWarning() << "The file index updated failed!";
    }
}

void IndexWorker::onFileCreated(const QString &file)
{
    if (d->isStoped)
        return;

    QDir dir;
    if (!dir.exists(d->indexStoragePath())) {
        if (!dir.mkpath(d->indexStoragePath())) {
            qWarning() << "Unable to create directory: " << d->indexStoragePath();
            return;
        }
    }

    try {
        // record spending
        QTime timer;
        timer.start();
        d->indexFileCount = 0;
        IndexWriterPtr writer = d->newIndexWriter(!d->indexExists());
        d->doIndexTask(writer, file, IndexWorkerPrivate::CreateIndex);
        writer->optimize();
        writer->close();

        qInfo() << "create index spending: " << timer.elapsed() << d->indexFileCount;
    } catch (const LuceneException &e) {
        qWarning() << QString::fromStdWString(e.getError());
    } catch (const std::exception &e) {
        qWarning() << QString(e.what());
    } catch (...) {
        qWarning() << "The file index created failed!";
    }
}

void IndexWorker::onFileDeleted(const QString &file)
{
    if (d->isStoped)
        return;

    try {
        qDebug() << "Delete file: [" << file << "]";
        IndexWriterPtr writer = d->newIndexWriter();

        QFileInfo info(file);
        if (info.isDir()) {
            TermPtr term = newLucene<Term>(L"path", (file + "/*").toStdWString());
            QueryPtr query = newLucene<WildcardQuery>(term);
            writer->deleteDocuments(query);
        } else {
            TermPtr term = newLucene<Term>(L"path", file.toStdWString());
            writer->deleteDocuments(term);
        }

        writer->optimize();
        writer->close();
    } catch (const LuceneException &e) {
        qWarning() << QString::fromStdWString(e.getError());
    } catch (const std::exception &e) {
        qWarning() << QString(e.what());
    } catch (...) {
        qWarning() << "The file index delete failed!";
    }
}

void IndexWorker::onCreateAllIndex()
{
    if (d->isStoped)
        return;

    if (d->indexExists()) {
        QTimer::singleShot(10 * 1000, this, &IndexWorker::onUpdateAllIndex);
        return;
    }

    onFileCreated(QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
    malloc_trim(0);
}

void IndexWorker::onUpdateAllIndex()
{
    if (d->isStoped)
        return;

    const auto &path = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
    try {
        IndexWriterPtr writer = d->newIndexWriter();
        d->doIndexTask(writer, path, IndexWorkerPrivate::UpdateIndex, true);
        writer->optimize();
        writer->close();
    } catch (const LuceneException &e) {
        qWarning() << QString::fromStdWString(e.getError());
    } catch (const std::exception &e) {
        qWarning() << QString(e.what());
    } catch (...) {
        qWarning() << "The file index updated failed!";
    }

    malloc_trim(0);
}

// SPDX-FileCopyrightText: 2023 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef INDEXWORKER_P_H
#define INDEXWORKER_P_H

#include "parser/abstractpropertyparser.h"

#include <lucene++/LuceneHeaders.h>

#include <QStandardPaths>
#include <QObject>
#include <QMap>

#include <QDebug>

class AbstractPropertyParser;
class IndexWorkerPrivate : public QObject
{
    Q_OBJECT
public:
    enum IndexType {
        CreateIndex,
        UpdateIndex
    };
    Q_ENUM(IndexType)

    explicit IndexWorkerPrivate(QObject *parent = nullptr);

    bool indexExists();
    bool isFilter(const QString &file);
    Lucene::IndexWriterPtr newIndexWriter(bool create = false);
    Lucene::IndexReaderPtr newIndexReader();

    inline static QString indexStoragePath()
    {
        static QString indexPath = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                + "/index";
        return indexPath;
    }

    void doIndexTask(const Lucene::IndexWriterPtr &writer, const QString &file, IndexType type, bool isCheck = false);
    void indexFile(Lucene::IndexWriterPtr writer, const QString &file, IndexType type);
    bool checkUpdate(const Lucene::IndexReaderPtr &reader, const QString &file, IndexType &type);
    Lucene::DocumentPtr indexDocument(const QString &file);
    QList<AbstractPropertyParser::Property> fileProperties(const QString &file);

    QMap<QString, AbstractPropertyParser *> propertyParsers;
    quint32 indexFileCount { 0 };
    std::atomic_bool isStoped { false };
};

#endif   // INDEXWORKER_P_H

// SPDX-FileCopyrightText: 2024 UnionTech Software Technology Co., Ltd.
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GLOBAL_DEFINE_H
#define GLOBAL_DEFINE_H

#include <QJsonObject>
#include <QMap>

typedef QJsonObject EmbeddingJsonObject;

#define FAISS_INDEX_FILES "~/vectorDatabase/IndexFilesPath.json"

#define EmbeddingDim 1024
#define SEARCH_RESULT_VERSION 1.0
#define GET_DOCS_VERSION 1.0
static constexpr char kSystemAssistantKey[] { "SystemAssistant" };
static constexpr char kGrandVectorSearch[] { "dde-grand-search" };
static constexpr char kUosAIAssistant[] { "uos-ai-assistant" };
static constexpr char kSystemAssistantData[] { "/usr/share/deepin-systemassistant-knowledge/deepin-knowledge/SystemAssistant" };

// DB
static constexpr char kEmbeddingDBMetaDataTable[] { "embedding_metadata" };
static constexpr char kEmbeddingDBIndexSegTable[] { "index_segment" };
static constexpr char kEmbeddingDBMetaDataTableID[] { "id" };
static constexpr char kEmbeddingDBMetaDataTableSource[] { "source" };
static constexpr char kEmbeddingDBMetaDataTableContent[] { "content" };

static constexpr char kEmbeddingDBSegIndexTableBitSet[] { "deleteBit" };
static constexpr char kEmbeddingDBSegIndexIndexName[] { "content" };

//index define
static constexpr char kFaissFlatIndex[] { "Flat" };
static constexpr char kFaissIvfFlatIndex[] { "IvfFlat" };
static constexpr char kFaissIvfPQIndex[] { "IvfPQ" };

//embedding define
static constexpr int kMaxChunksSize = 300;
static constexpr int kMinChunksSize = 200;

//文档分块后的存储结构
struct Document {
    QString content;
    QString source;

    //int64_t startIndex; 文本块在文档内的起始索引(字符位置)，可以用于source定位。
    explicit Document(const QString &content = "",
                      const QString &source = "")
            : content(content),
              source(source) {}

    //Dynamic_cast
    virtual ~Document();
};

// 索引创建状态
#define INDEX_STATUS_SUCCESS_CODE 1
#define INDEX_STATUS_CREATING_CODE 2
#define INDEX_STATUS_DOCERROR_CODE -1
#define INDEX_STATUS_DATAERROR_CODE -2

#define GET_INDEX_STATUS_CODE(status_info) (status_info##_CODE)
#endif // GLOBAL_DEFINE_H



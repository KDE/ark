/*
    SPDX-FileCopyrightText: 2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "jsonparser.h"
#include "archiveinterface.h"

#include <QDebug>
#include <QJsonDocument>

JSONParser::JSONParser()
{
}

JSONParser::~JSONParser()
{
}

JSONParser::JSONArchive JSONParser::parse(QIODevice *json)
{
    QJsonParseError error;
    QJsonDocument jsonDoc = QJsonDocument::fromJson(json->readAll(), &error);

    if (error.error != QJsonParseError::NoError) {
        qDebug() << "Parse error: " << error.errorString();
        return JSONParser::JSONArchive();
    }

    return createJSONArchive(jsonDoc.toVariant());
}

JSONParser::JSONArchive JSONParser::createJSONArchive(const QVariant &json)
{
    JSONParser::JSONArchive archive;

    const auto jsonList = json.toList();
    for (const QVariant &entry : jsonList) {
        const QVariantMap entryMap = entry.toMap();

        if (!entryMap.contains(QStringLiteral("fullPath"))) {
            continue;
        }

        auto e = std::make_unique<Kerfuffle::Archive::Entry>();

        QVariantMap::const_iterator entryIterator = entryMap.constBegin();
        for (; entryIterator != entryMap.constEnd(); ++entryIterator) {
            const QByteArray key = entryIterator.key().toUtf8();
            if (e->property(key.constData()).isValid()) {
                e->setProperty(key.constData(), entryIterator.value());
            } else {
                qDebug() << entryIterator.key() << "is not a valid entry key";
            }
        }

        const QString fullPath = entryMap[QStringLiteral("fullPath")].toString();
        archive[fullPath] = std::move(e);
    }

    return archive;
}

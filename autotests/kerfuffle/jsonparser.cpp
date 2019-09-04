/*
 * Copyright (c) 2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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

        if (!entryMap.contains(QLatin1String("fullPath"))) {
            continue;
        }

        Kerfuffle::Archive::Entry *e = new Kerfuffle::Archive::Entry();

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
        archive[fullPath] = e;
    }

    return archive;
}

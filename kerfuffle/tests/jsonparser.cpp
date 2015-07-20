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
#include "kerfuffle/archiveinterface.h"

#include <QDebug>
#include <QJsonDocument>
#include <QLatin1String>

typedef QMap<QString, Kerfuffle::EntryMetaDataType> ArchiveProperties;

static ArchiveProperties archiveProperties()
{
    static ArchiveProperties properties;

    if (!properties.isEmpty()) {
        return properties;
    }

    properties[QLatin1String("FileName")]            = Kerfuffle::FileName;
    properties[QLatin1String("InternalID")]          = Kerfuffle::InternalID;
    properties[QLatin1String("Permissions")]         = Kerfuffle::Permissions;
    properties[QLatin1String("Owner")]               = Kerfuffle::Owner;
    properties[QLatin1String("Group")]               = Kerfuffle::Group;
    properties[QLatin1String("Size")]                = Kerfuffle::Size;
    properties[QLatin1String("CompressedSize")]      = Kerfuffle::CompressedSize;
    properties[QLatin1String("Link")]                = Kerfuffle::Link;
    properties[QLatin1String("Ratio")]               = Kerfuffle::Ratio;
    properties[QLatin1String("CRC")]                 = Kerfuffle::CRC;
    properties[QLatin1String("Method")]              = Kerfuffle::Method;
    properties[QLatin1String("Version")]             = Kerfuffle::Version;
    properties[QLatin1String("Timestamp")]           = Kerfuffle::Timestamp;
    properties[QLatin1String("IsDirectory")]         = Kerfuffle::IsDirectory;
    properties[QLatin1String("Comment")]             = Kerfuffle::Comment;
    properties[QLatin1String("IsPasswordProtected")] = Kerfuffle::IsPasswordProtected;

    return properties;
}

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
    static const ArchiveProperties properties = archiveProperties();

    JSONParser::JSONArchive archive;

    foreach (const QVariant &entry, json.toList()) {
        const QVariantMap entryMap = entry.toMap();

        if (!entryMap.contains(QLatin1String("FileName"))) {
            continue;
        }

        Kerfuffle::ArchiveEntry archiveEntry;

        QVariantMap::const_iterator entryIterator = entryMap.constBegin();
        for (; entryIterator != entryMap.constEnd(); ++entryIterator) {
            if (properties.contains(entryIterator.key())) {
                archiveEntry[properties[entryIterator.key()]] = entryIterator.value();
            } else {
                qDebug() << entryIterator.key() << "is not a valid entry key";
            }
        }

        const QString fileName = entryMap[QLatin1String("FileName")].toString();
        archive[fileName] = archiveEntry;
    }

    return archive;
}

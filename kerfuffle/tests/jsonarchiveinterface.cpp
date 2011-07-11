/*
 * Copyright (c) 2010 Raphael Kubo da Costa <kubito@gmail.com>
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

#include "jsonarchiveinterface.h"

#include <kdebug.h>
#include <qjson/parser.h>

#include <qfile.h>

JSONArchiveInterface::JSONArchiveInterface(QObject *parent, const QVariantList& args)
    : Kerfuffle::ReadWriteArchiveInterface(parent, args)
{
}

JSONArchiveInterface::~JSONArchiveInterface()
{
}

bool JSONArchiveInterface::list()
{
    foreach (const Kerfuffle::ArchiveEntry& e, m_entryList) {
        entry(e);
    }

    return true;
}

bool JSONArchiveInterface::open()
{
    QFile file(filename());

    if (!file.exists()) {
        return false;
    }

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    bool ok;
    QJson::Parser parser;

    const QVariant json = parser.parse(&file, &ok);

    if (!ok) {
        kDebug() << filename() << ":"  << parser.errorLine() << ":"
                 << parser.errorString();
        return false;
    }

    if (!parseJson(json)) {
        return false;
    }

    return true;
}

bool JSONArchiveInterface::parseJson(const QVariant& json)
{
    QMap<QString, Kerfuffle::EntryMetaDataType> valueMap;
    valueMap[QLatin1String("FileName")] = Kerfuffle::FileName;
    valueMap[QLatin1String("InternalID")] = Kerfuffle::InternalID;
    valueMap[QLatin1String("Permissions")] = Kerfuffle::Permissions;
    valueMap[QLatin1String("Owner")] = Kerfuffle::Owner;
    valueMap[QLatin1String("Group")] = Kerfuffle::Group;
    valueMap[QLatin1String("Size")] = Kerfuffle::Size;
    valueMap[QLatin1String("CompressedSize")] = Kerfuffle::CompressedSize;
    valueMap[QLatin1String("Link")] = Kerfuffle::Link;
    valueMap[QLatin1String("Ratio")] = Kerfuffle::Ratio;
    valueMap[QLatin1String("CRC")] = Kerfuffle::CRC;
    valueMap[QLatin1String("Method")] = Kerfuffle::Method;
    valueMap[QLatin1String("Version")] = Kerfuffle::Version;
    valueMap[QLatin1String("Timestamp")] = Kerfuffle::Timestamp;
    valueMap[QLatin1String("IsDirectory")] = Kerfuffle::IsDirectory;
    valueMap[QLatin1String("Comment")] = Kerfuffle::Comment;
    valueMap[QLatin1String("IsPasswordProtected")] = Kerfuffle::IsPasswordProtected;

    foreach (const QVariant& entry, json.toList()) {
        const QVariantMap entryMap = entry.toMap();

        if (!entryMap.contains(QLatin1String("FileName"))) {
            continue;
        }

        Kerfuffle::ArchiveEntry e;

        QVariantMap::const_iterator entryIterator = entryMap.constBegin();
        for (; entryIterator != entryMap.constEnd(); ++entryIterator) {
            if (valueMap.contains(entryIterator.key())) {
                e[valueMap[entryIterator.key()]] = entryIterator.value();
            } else {
                kDebug() << entryIterator.key() << "is not a valid entry key";
            }
        }

        m_entryNameList.append(e[Kerfuffle::FileName].toString());
        m_entryList.append(e);
    }

    return true;
}

bool JSONArchiveInterface::addFiles(const QStringList& files, const Kerfuffle::CompressionOptions& options)
{
    Q_UNUSED(options)

    QStringList entryNameList;
    QList<Kerfuffle::ArchiveEntry> entryList;

    foreach (const QString& file, files) {
        if (m_entryNameList.contains(file)) {
            return false;
        }

        Kerfuffle::ArchiveEntry e;
        e[Kerfuffle::FileName] = file;

        entryNameList.append(e[Kerfuffle::FileName].toString());
        entryList.append(e);
    }

    m_entryNameList.append(entryNameList);
    m_entryList.append(entryList);

    return true;
}

bool JSONArchiveInterface::copyFiles(const QList<QVariant>& files, const QString& destinationDirectory, Kerfuffle::ExtractionOptions options)
{
    Q_UNUSED(files)
    Q_UNUSED(destinationDirectory)
    Q_UNUSED(options)

    return true;
}

bool JSONArchiveInterface::deleteFiles(const QList<QVariant>& files)
{
    QStringList fileList;
    foreach (const QVariant& file, files) {
        fileList.append(file.toString());
    }

    QList<Kerfuffle::ArchiveEntry> newEntryList;
    QStringList newEntryNameList;
    foreach (const Kerfuffle::ArchiveEntry& e, m_entryList) {
        if (fileList.contains(e[Kerfuffle::FileName].toString())) {
            continue;
        }

        newEntryList.append(e);
        newEntryNameList.append(e[Kerfuffle::FileName].toString());
    }

    m_entryNameList = newEntryNameList;
    m_entryList = newEntryList;

    return true;
}

#include "jsonarchiveinterface.moc"

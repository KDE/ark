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

#include "textarchiveinterface.h"

#include <qfile.h>

TextArchiveInterface::TextArchiveInterface(QObject *parent, const QVariantList& args)
    : Kerfuffle::ReadWriteArchiveInterface(parent, args)
{
}

TextArchiveInterface::~TextArchiveInterface()
{
}

bool TextArchiveInterface::list()
{
    foreach (const Kerfuffle::ArchiveEntry& e, m_entryList) {
        entry(e);
    }

    return true;
}

bool TextArchiveInterface::open()
{
    QFile file(filename());

    if (!file.exists())
        return false;

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;

    while (!file.atEnd()) {
        const QByteArray rawLine(file.readLine());
        const QString line(rawLine.left(rawLine.length() - 1));

        Kerfuffle::ArchiveEntry e(stringToArchiveEntry(line));

        m_entryNameList.append(e[Kerfuffle::FileName].toString());
        m_entryList.append(e);
    }

    return true;
}

bool TextArchiveInterface::addFiles(const QStringList& files, const Kerfuffle::CompressionOptions& options)
{
    QStringList entryNameList;
    QList<Kerfuffle::ArchiveEntry> entryList;

    foreach (const QString& file, files) {
        if (m_entryNameList.contains(file))
            return false;

        Kerfuffle::ArchiveEntry e(stringToArchiveEntry(file));

        entryNameList.append(e[Kerfuffle::FileName].toString());
        entryList.append(e);
    }

    m_entryNameList.append(entryNameList);
    m_entryList.append(entryList);

    return true;
}

bool TextArchiveInterface::copyFiles(const QList<QVariant>& files, const QString& destinationDirectory, Kerfuffle::ExtractionOptions options)
{
    return true;
}

bool TextArchiveInterface::deleteFiles(const QList<QVariant>& files)
{
    return true;
}

Kerfuffle::ArchiveEntry TextArchiveInterface::stringToArchiveEntry(const QString& entry)
{
    Kerfuffle::ArchiveEntry e;

    e[Kerfuffle::FileName] = entry;
    e[Kerfuffle::IsDirectory] = entry.endsWith('/');

    return e;
}

#include "textarchiveinterface.moc"

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

#ifndef JSONARCHIVEINTERFACE_H
#define JSONARCHIVEINTERFACE_H

#include "kerfuffle/archive.h"
#include "kerfuffle/archiveinterface.h"

/**
 * A dummy archive interface used by our test cases.
 *
 * It reads a JSON file which defines the contents of the archive.
 *
 * The file consists of a series of pairs which describe various aspects of an
 * archive. For now, there is a single top-level pair whose key is "entries" and
 * whose value is an array containing a list of objects whose keys are values
 * from the EntryMetaDataType enum.
 * The only required key for each entry is FileName; other values which are
 * omitted for each entry are assumed to be 0 or false.
 *
 * The file's content is read to memory when open() is called and the archive
 * is then closed. This means that this class never changes the file's content
 * on disk, and entry addition or deletion do not change the original file.
 *
 * Example file:
 * @code
 * {
 *     "entries": [
 *         { "FileName": "foo", "IsPasswordProtected": true },
 *         { "FileName": "aDir/", "IsDirectory": true }
 *     ]
 * }
 * @endcode
 *
 * @author Raphael Kubo da Costa <kubito@gmail.com>
 */
class JSONArchiveInterface : public Kerfuffle::ReadWriteArchiveInterface
{
    Q_OBJECT

public:
    explicit JSONArchiveInterface(QObject *parent, const QVariantList& args);
    virtual ~JSONArchiveInterface();

    virtual bool list();
    virtual bool open();

    virtual bool addFiles(const QStringList& files, const Kerfuffle::CompressionOptions& options);
    virtual bool copyFiles(const QList<QVariant>& files, const QString& destinationDirectory, Kerfuffle::ExtractionOptions options);
    virtual bool deleteFiles(const QList<QVariant>& files);

private:
    /**
     * Parses each entry in the QVariantMap obtained from parsing a JSON file and
     * creates a Kerfuffle::ArchiveEntry for it, adding it to the internal entry list.
     *
     * If an entry does not have a "FileName" key, it is ignored. Keys which do not correspond
     * to a value in the EntryMetaDataType enum are ignored.
     *
     * @retval true  The map was parsed correctly.
     * @retval false The map does not have an "entries" key, or an error has occurred.
     */
    bool parseJsonMap(const QVariantMap& jsonMap);

    QStringList m_entryNameList;
    QList<Kerfuffle::ArchiveEntry> m_entryList;
};

#endif

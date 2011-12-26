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

#ifndef JSONPARSER_H
#define JSONPARSER_H

#include "kerfuffle/archive.h"

#include <QtCore/QIODevice>
#include <QtCore/QMap>
#include <QtCore/QString>

/**
 * Simple parser which reads JSON files and creates @c ArchiveEntry objects
 * from it.
 *
 * The JSON file is expected to follow a specific format that describes an
 * archive read by Kerfuffle.
 *
 * The format consists of a list of dictionaries whose keys are values from the
 * EntryMetaDataType enum.  The only required key for each entry is FileName;
 * other values which are omitted for each entry are assumed to be 0 or false.
 *
 * Example file:
 * @code
 * [
 *     { "FileName": "foo", "IsPasswordProtected": true },
 *     { "FileName": "aDir/", "IsDirectory": true }
 * ]
 * @endcode
 *
 * @author Raphael Kubo da Costa <rakuco@FreeBSD.org>
 */
class JSONParser
{
public:
    typedef QMap<QString, Kerfuffle::ArchiveEntry> JSONArchive;

    ~JSONParser();

    static JSONArchive parse(const QString &json);
    static JSONArchive parse(QIODevice *json);

private:
    JSONParser();

    /**
     * Parses each entry in the QVariant obtained from parsing a JSON file and
     * creates a @c JSONArchive from them.
     *
     * If an entry does not have a "FileName" key, it is ignored. Keys which do
     * not correspond to a value in the EntryMetaDataType enum are ignored.
     *
     * @return A new @c JSONArchive corresponding to the parsed JSON file. If a
     *         parsing error occurs, it is empty.
     */
    static JSONArchive createJSONArchive(const QVariant &json);
};

#endif // JSONPARSER_H

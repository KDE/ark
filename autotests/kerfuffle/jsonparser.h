/*
    SPDX-FileCopyrightText: 2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef JSONPARSER_H
#define JSONPARSER_H

#include "archiveentry.h"

#include <QIODevice>
#include <QMap>

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
 *     { "fullPath": "foo", "IsPasswordProtected": true },
 *     { "fullPath": "aDir/", "IsDirectory": true }
 * ]
 * @endcode
 *
 * @author Raphael Kubo da Costa <rakuco@FreeBSD.org>
 */
class JSONParser
{
public:
    typedef QMap<QString, Kerfuffle::Archive::Entry*> JSONArchive;

    ~JSONParser();

    static JSONArchive parse(QIODevice *json);

private:
    JSONParser();

    /**
     * Parses each entry in the QVariant obtained from parsing a JSON file and
     * creates a @c JSONArchive from them.
     *
     * If an entry does not have a "fullPath" key, it is ignored. Keys which do
     * not correspond to a value in the EntryMetaDataType enum are ignored.
     *
     * @return A new @c JSONArchive corresponding to the parsed JSON file. If a
     *         parsing error occurs, it is empty.
     */
    static JSONArchive createJSONArchive(const QVariant &json);
};

#endif // JSONPARSER_H

/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2010 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (c) 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */


#ifndef CLIPLUGIN_H
#define CLIPLUGIN_H

#include "cliinterface.h"

class CliPlugin : public Kerfuffle::CliInterface
{
    Q_OBJECT

public:
    explicit CliPlugin(QObject *parent, const QVariantList & args);
    ~CliPlugin() override;

    void resetParsing() override;
    bool readListLine(const QString &line) override;
    bool readExtractLine(const QString &line) override;
    bool readDeleteLine(const QString &line) override;

private:
    enum ArchiveType {
        ArchiveType7z = 0,
        ArchiveTypeBZip2,
        ArchiveTypeGZip,
        ArchiveTypeXz,
        ArchiveTypeTar,
        ArchiveTypeZip,
        ArchiveTypeRar
    } m_archiveType;

    enum ParseState {
        ParseStateTitle = 0,
        ParseStateHeader,
        ParseStateArchiveInformation,
        ParseStateComment,
        ParseStateEntryInformation
    } m_parseState;

    void setupCliProperties();
    void handleMethods(const QStringList &methods);
    void fixDirectoryFullName();

    int m_linesComment;
    Kerfuffle::Archive::Entry *m_currentArchiveEntry;
    bool m_isFirstInformationEntry;
};

#endif // CLIPLUGIN_H

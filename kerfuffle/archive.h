/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
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
#ifndef KERFUFFLE_ARCHIVE_H
#define KERFUFFLE_ARCHIVE_H

#include "kerfuffle_export.h"

#include <QString>
#include <QStringList>
#include <QHash>

#include <KUrl>

class KJob;

namespace Kerfuffle
{
class ListJob;
class ExtractJob;
class DeleteJob;
class AddJob;

enum EntryMetaDataType { FileName = 0, InternalID = 1, Permissions = 2, Owner = 3,
                         Group = 4, Size = 5, CompressedSize = 6, Link = 7, Ratio = 8,
                         CRC = 9, Method = 10, Version = 11, Timestamp = 12, IsDirectory = 13, Comment = 14, IsPasswordProtected = 15, Custom = 1048576
                       };

typedef QHash<int, QVariant> ArchiveEntry;

/**
These are the extra options for doing the compression. Naming convention
is CamelCase with either Global, or the compression type (such as Zip,
Rar, etc), followed by the property name used
 */
typedef QHash<QString, QVariant> CompressionOptions;
typedef QHash<QString, QVariant> ExtractionOptions;

class KERFUFFLE_EXPORT Archive
{
public:

    virtual ~Archive() {}

    virtual QString fileName() = 0;
    virtual bool isReadOnly() = 0;

    virtual KJob*       open() = 0;
    virtual KJob*       create() = 0;
    virtual ListJob*    list() = 0;
    virtual DeleteJob*  deleteFiles(const QList<QVariant> & files) = 0;

    /**
     * Compression options that should be handled by all interfaces:
     *
     * GlobalWorkDir - Change to this dir before adding the new files.
     * The path names should then be added relative to this directory.
     *
     * TODO: find a way to actually add files to specific locations in
     * the archive
     * (not supported yet) GlobalPathInArchive - a path relative to the
     * archive root where the files will be added under
     *
     */
    virtual AddJob*     addFiles(const QStringList & files, const CompressionOptions& options = CompressionOptions()) = 0;

    virtual ExtractJob* copyFiles(const QList<QVariant> & files, const QString & destinationDir, ExtractionOptions options = ExtractionOptions()) = 0;

    virtual bool isSingleFolderArchive() = 0;
    virtual QString subfolderName() = 0;
    virtual bool isPasswordProtected() = 0;

    virtual void setPassword(QString password) = 0;

};

KERFUFFLE_EXPORT Archive* factory(const QString & filename, const QString & requestedMimeType = QString());
KERFUFFLE_EXPORT QStringList supportedMimeTypes();
KERFUFFLE_EXPORT QStringList supportedWriteMimeTypes();
} // namespace Kerfuffle


#endif // KERFUFFLE_ARCHIVE_H

/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
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

#ifndef READWRITELIBARCHIVEPLUGIN_H
#define READWRITELIBARCHIVEPLUGIN_H

#include "libarchiveplugin.h"

#include <QDir>
#include <QStringList>

using namespace Kerfuffle;

class ReadWriteLibarchivePlugin : public LibarchivePlugin
{
    Q_OBJECT

public:
    explicit ReadWriteLibarchivePlugin(QObject *parent, const QVariantList& args);
    ~ReadWriteLibarchivePlugin();

    bool addFiles(const QList<Archive::Entry*> &files, const Archive::Entry *destination, const CompressionOptions& options) Q_DECL_OVERRIDE;
    bool moveFiles(const QList<Archive::Entry*> &files, const Archive::Entry *destination, const CompressionOptions& options) Q_DECL_OVERRIDE;
    bool deleteFiles(const QList<Archive::Entry*>& files) Q_DECL_OVERRIDE;

private:
    bool initializeNewFileWriter(const ArchiveWrite &archiveWrite, const CompressionOptions &options);
    bool initializeWriter(const ArchiveWrite &archiveWrite, const ArchiveRead &archiveRead);
    bool copyOldEntries(const ArchiveWrite &archiveWrite, const ArchiveRead &archiveRead, const QStringList &filePaths, int &entriesCounter, bool deleteMode = false);
    bool writeFile(const QString& relativeName, const QString& destination, struct archive* arch);

    QStringList m_writtenFiles;
};

#endif // READWRITELIBARCHIVEPLUGIN_H

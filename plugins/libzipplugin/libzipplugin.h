/*
 * Copyright (c) 2016 Ragnar Thomsen <rthomsen6@gmail.com>
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

#ifndef LIBZIPPLUGIN_H
#define LIBZIPPLUGIN_H

#include "kerfuffle/archiveinterface.h"

using namespace Kerfuffle;

class LibzipPlugin : public ReadWriteArchiveInterface
{
    Q_OBJECT

public:
    explicit LibzipPlugin(QObject *parent, const QVariantList& args);
    virtual ~LibzipPlugin();

    virtual bool list() Q_DECL_OVERRIDE;
    virtual bool doKill() Q_DECL_OVERRIDE;
    virtual bool copyFiles(const QVariantList& files, const QString& destinationDirectory, const ExtractionOptions& options) Q_DECL_OVERRIDE;

    virtual bool addFiles(const QStringList& files, const CompressionOptions& options) Q_DECL_OVERRIDE;
    virtual bool deleteFiles(const QList<QVariant>& files) Q_DECL_OVERRIDE;
    virtual bool addComment(const QString& comment) Q_DECL_OVERRIDE;
    virtual bool testArchive() Q_DECL_OVERRIDE;
};

#endif // LIBZIPPLUGIN_H

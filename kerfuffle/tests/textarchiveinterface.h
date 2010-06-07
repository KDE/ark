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

#ifndef TEXTARCHIVEINTERFACE_H
#define TEXTARCHIVEINTERFACE_H

#include "kerfuffle/archive.h"
#include "kerfuffle/archiveinterface.h"

/**
 * A dummy archive interface used by our test cases.
 * It reads a plain-text file consisting of one entry per line.
 * The file's content is read to memory, and entry addition and
 * deletion do not change the original file on disk.
 *
 * @author Raphael Kubo da Costa <kubito@gmail.com>
 */
class TextArchiveInterface : public Kerfuffle::ReadWriteArchiveInterface
{
    Q_OBJECT

public:
    explicit TextArchiveInterface(QObject *parent, const QVariantList& args);
    virtual ~TextArchiveInterface();

    virtual bool list();
    virtual bool open();

    virtual bool addFiles(const QStringList& files, const Kerfuffle::CompressionOptions& options);
    virtual bool copyFiles(const QList<QVariant>& files, const QString& destinationDirectory, Kerfuffle::ExtractionOptions options);
    virtual bool deleteFiles(const QList<QVariant>& files);

private:
    QList<Kerfuffle::ArchiveEntry> m_entryList;
};

#endif

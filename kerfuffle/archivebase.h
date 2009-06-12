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

#ifndef ARCHIVEBASE_H
#define ARCHIVEBASE_H

#include "archive.h"
#include "archiveinterface.h"
#include "jobs.h"
#include "kerfuffle_export.h"

namespace Kerfuffle
{
class KERFUFFLE_EXPORT ArchiveBase: public QObject, public Archive
{
    Q_OBJECT
public:
    /*
     * Creates an Arch to operate on the given interface.
     * This takes ownership of the interface, which is deleted
     * on the destructor.
     */
    ArchiveBase(ReadOnlyArchiveInterface *archive);
    virtual ~ArchiveBase();

    virtual KJob* open();
    virtual KJob* create();
    virtual ListJob* list();
    virtual DeleteJob* deleteFiles(const QList<QVariant> & files);
    virtual AddJob* addFiles(const QStringList & files, const CompressionOptions& options = CompressionOptions());
    virtual ExtractJob* copyFiles(const QList<QVariant> & files, const QString & destinationDir, ExtractionOptions options);

    virtual bool isReadOnly();
    virtual QString fileName();

    virtual bool isSingleFolderArchive();
    virtual QString subfolderName();
    virtual bool isPasswordProtected();

    virtual void setPassword(QString password);

private slots:
    void onListFinished(KJob*);
    void onAddFinished(KJob*);

private:
    void listIfNotListed();
    ReadOnlyArchiveInterface *m_iface;
    bool m_hasBeenListed;
    bool m_isSingleFolderArchive;
    bool m_isPasswordProtected;
    QString m_subfolderName;
    qlonglong m_extractedFilesSize;

};
} // namespace Kerfuffle

#endif // ARCHIVEBASE_H

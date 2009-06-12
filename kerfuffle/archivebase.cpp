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

#include "archivebase.h"
#include "queries.h"

#include <QDateTime>
#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QList>
#include <QStringList>

#include <KDebug>

namespace Kerfuffle
{
ArchiveBase::ArchiveBase(ReadOnlyArchiveInterface *archive)
        : QObject(), Archive(), m_iface(archive),
        m_hasBeenListed(false),
        m_isSingleFolderArchive(false)
{
    Q_ASSERT(archive);
    archive->setParent(this);
    //setReadOnly( archive->isReadOnly() );
}

ArchiveBase::~ArchiveBase()
{
    delete m_iface;
    m_iface = 0;
}

bool ArchiveBase::isReadOnly()
{
    return m_iface->isReadOnly();
}

KJob* ArchiveBase::open()
{
    return 0;
}

KJob* ArchiveBase::create()
{
    return 0;
}

ListJob* ArchiveBase::list()
{
    ListJob *job = new ListJob(m_iface, this);

    //if this job has not been listed before, we grab the opportunity to
    //collect some information about the archive
    if (!m_hasBeenListed) {
        connect(job, SIGNAL(result(KJob*)),
                this, SLOT(onListFinished(KJob*)));
    }
    return job;
}

DeleteJob* ArchiveBase::deleteFiles(const QList<QVariant> & files)
{
    if (m_iface->isReadOnly()) {
        return 0;
    }
    DeleteJob *newJob = new DeleteJob(files, static_cast<ReadWriteArchiveInterface*>(m_iface), this);

    return newJob;
}

AddJob* ArchiveBase::addFiles(const QStringList & files, const CompressionOptions& options)
{
    Q_ASSERT(!m_iface->isReadOnly());
    AddJob *newJob = new AddJob(files, options, static_cast<ReadWriteArchiveInterface*>(m_iface), this);
    connect(newJob, SIGNAL(result(KJob*)),
            this, SLOT(onAddFinished(KJob*)));
    return newJob;
}

ExtractJob* ArchiveBase::copyFiles(const QList<QVariant> & files, const QString & destinationDir, ExtractionOptions options)
{
    ExtractionOptions newOptions = options;
    if (isPasswordProtected())
        newOptions["PasswordProtectedHint"] = true;

    ExtractJob *newJob = new ExtractJob(files, destinationDir, newOptions, m_iface, this);
    return newJob;
}

QString ArchiveBase::fileName()
{
    return m_iface->filename();
}

void ArchiveBase::onAddFinished(KJob* job)
{
    //if the archive was previously a single folder archive and an add job
    //has successfully finished, then it is no longer a single folder
    //archive (for the current implementation, which does not allow adding
    //folders/files other places than the root.
    //TODO: handle the case of creating a new file and singlefolderarchive
    //then.
    if (m_isSingleFolderArchive && !job->error()) {
        m_isSingleFolderArchive = false;
    }
}

void ArchiveBase::onListFinished(KJob* job)
{
    ListJob *ljob = qobject_cast<ListJob*>(job);
    m_extractedFilesSize = ljob->extractedFilesSize();
    m_isSingleFolderArchive = ljob->isSingleFolderArchive();
    m_isPasswordProtected = ljob->isPasswordProtected();
    m_subfolderName = ljob->subfolderName();
    if (m_subfolderName.isEmpty()) {
        QFileInfo fi(fileName());
        QString base = fi.completeBaseName();

        //special case for tar.gz/bzip2 files
        if (base.right(4).toUpper() == ".TAR")
            base.chop(4);

        m_subfolderName = base;
    }

    m_hasBeenListed = true;
}

void ArchiveBase::listIfNotListed()
{
    if (!m_hasBeenListed) {
        KJob *job = list();

        QEventLoop loop(this);

        connect(job, SIGNAL(result(KJob*)),
                &loop, SLOT(quit()));
        job->start();
        loop.exec();
    }
}

bool ArchiveBase::isSingleFolderArchive()
{
    listIfNotListed();
    return m_isSingleFolderArchive;
}

bool ArchiveBase::isPasswordProtected()
{
    listIfNotListed();
    return m_isPasswordProtected;
}

QString ArchiveBase::subfolderName()
{
    listIfNotListed();
    return m_subfolderName;
}

void ArchiveBase::setPassword(QString password)
{
    m_iface->setPassword(password);
}


} // namespace Kerfuffle

#include "archivebase.moc"

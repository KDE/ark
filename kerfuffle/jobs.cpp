/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008-2009 Harald Hvaal <haraldhv )@@@( stud(dot)ntnu.no>
 * Copyright (c) 2009 Raphael Kubo da Costa <kubito@gmail.com>
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

#include "jobs.h"
#include "threading.h"

#include <QApplication>
#include <QDir>
#include <QTimer>

#include <KDebug>
#include <KLocale>

//#define KERFUFFLE_NOJOBTHREADING

namespace Kerfuffle
{
Job::Job(ReadOnlyArchiveInterface *interface, QObject *parent)
        : KJob(parent)
        , m_interface(interface)
        , m_workerThread(0)
{
    static bool onlyOnce = false;
    if (!onlyOnce) {
        qRegisterMetaType<QPair<QString, QString> >("QPair<QString,QString>");
        onlyOnce = true;
    }

    setCapabilities(KJob::Killable);
}

Job::~Job()
{
#ifndef KERFUFFLE_NOJOBTHREADING
    m_workerThread->wait();
    delete m_workerThread;
    m_workerThread = 0;
#endif
}

void Job::start()
{
#ifdef KERFUFFLE_NOJOBTHREADING
    QTimer::singleShot(0, this, SLOT(doWork()));
#else
    m_workerThread = new ThreadExecution(this);
    m_workerThread->start();
#endif
}

void Job::onError(const QString & message, const QString & details)
{
    Q_UNUSED(details);

    setError(1);
    setErrorText(message);
}

void Job::onEntry(const ArchiveEntry & archiveEntry)
{
    emit newEntry(archiveEntry);
}

void Job::onProgress(double value)
{
    setPercent(static_cast<unsigned long>(100.0*value));
}

void Job::onInfo(const QString& info)
{
    emit infoMessage(this, info);
}

void Job::onEntryRemoved(const QString & path)
{
    emit entryRemoved(path);
}

void Job::onFinished(bool result)
{
    kDebug(1601);
    m_interface->removeObserver(this);

    setError(!result);

    if (errorString().isEmpty())
        setErrorText(i18n("An error occurred while performing the operation."));

    emitResult();
}

void Job::onUserQuery(Query *query)
{
    emit userQuery(query);
}

bool Job::doKill()
{
    kDebug(1601);
    bool ret = m_interface->doKill();
    if (!ret)
        kDebug(1601) << "Killing does not seem to be supported here.";
    return ret;
}

ListJob::ListJob(ReadOnlyArchiveInterface *interface, QObject *parent)
        : Job(interface, parent),
        m_isSingleFolderArchive(true),
        m_isPasswordProtected(false),
        m_extractedFilesSize(0)
{
    connect(this, SIGNAL(newEntry(const ArchiveEntry&)),
            this, SLOT(onNewEntry(const ArchiveEntry&)));
}

void ListJob::doWork()
{
    emit description(this, i18n("Loading archive..."));
    m_interface->registerObserver(this);
    bool ret = m_interface->list();

    if (!m_interface->waitForFinishedSignal()) m_interface->finished(ret);
}

void ListJob::onNewEntry(const ArchiveEntry& entry)
{
    m_extractedFilesSize += entry[ Size ].toLongLong();
    m_isPasswordProtected |= entry [ IsPasswordProtected ].toBool();

    QString filename = entry[ FileName ].toString();
    QString fileBaseRoot = filename.split(QDir::separator()).first();

    if (m_previousEntry.isEmpty()) { // Set the root path of the filename
        m_previousEntry = fileBaseRoot;
        m_subfolderName = fileBaseRoot;
        m_isSingleFolderArchive = entry[ IsDirectory ].toBool();
    } else {
        if (m_previousEntry != fileBaseRoot) {
            m_isSingleFolderArchive = false;
            m_subfolderName.clear();
        } else {
            // The state may change only if the folder's files were added before itself
            if (entry[ IsDirectory ].toBool())
                m_isSingleFolderArchive = true;
        }
    }
}

ExtractJob::ExtractJob(const QList<QVariant>& files, const QString& destinationDir,
                       ExtractionOptions options, ReadOnlyArchiveInterface *interface, QObject *parent)
        : Job(interface,  parent), m_files(files), m_destinationDir(destinationDir), m_options(options)
{
}

void ExtractJob::doWork()
{
    QString desc;
    if (m_files.count() == 0) {
        desc = i18n("Extracting all files");
    } else {
        desc = i18np("Extracting one file", "Extracting %1 files", m_files.count());
    }
    emit description(this, desc);

    m_interface->registerObserver(this);

    fillInDefaultValues(m_options);

    kDebug(1601) << "Starting extraction with selected files "
    << m_files
    << " Destination dir " << m_destinationDir
    << " And options " << m_options
    ;

    bool ret = m_interface->copyFiles(m_files, m_destinationDir, m_options);

    if (!m_interface->waitForFinishedSignal()) m_interface->finished(ret);
}

void ExtractJob::fillInDefaultValues(ExtractionOptions& options)
{
    if (!options.contains("PreservePaths")) {
        options["PreservePaths"] = false;
    }
}

AddJob::AddJob(const QStringList& files, const CompressionOptions& options , ReadWriteArchiveInterface *interface, QObject *parent)
        : Job(interface, parent), m_files(files), m_options(options)
{
    kDebug(1601);
}

void AddJob::doWork()
{
    emit description(this, i18np("Adding a file", "Adding %1 files", m_files.count()));

    ReadWriteArchiveInterface *m_writeInterface =
        qobject_cast<ReadWriteArchiveInterface*>
        (m_interface);

    Q_ASSERT(m_writeInterface);

    m_writeInterface->registerObserver(this);
    bool ret = m_writeInterface->addFiles(m_files, m_options);

    if (!m_interface->waitForFinishedSignal()) m_interface->finished(ret);
}

DeleteJob::DeleteJob(const QList<QVariant>& files, ReadWriteArchiveInterface *interface, QObject *parent)
        : Job(interface, parent), m_files(files)
{
}

void DeleteJob::doWork()
{
    emit description(this, i18np("Deleting a file from the archive", "Deleting %1 files", m_files.count()));

    ReadWriteArchiveInterface *m_writeInterface =
        qobject_cast<ReadWriteArchiveInterface*>
        (m_interface);

    Q_ASSERT(m_writeInterface);

    m_writeInterface->registerObserver(this);
    int ret = m_writeInterface->deleteFiles(m_files);

    if (!m_interface->waitForFinishedSignal()) m_interface->finished(ret);
}
} // namespace Kerfuffle

#include "jobs.moc"

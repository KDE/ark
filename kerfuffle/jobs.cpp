/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (c) 2009-2012 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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

#include <QThread>

#include <KDebug>
#include <KLocale>

//#define DEBUG_RACECONDITION

namespace Kerfuffle
{

class Job::Private : public QThread
{
public:
    Private(Job *job, QObject *parent = 0)
        : QThread(parent)
        , q(job)
    {
        connect(q, SIGNAL(result(KJob*)), SLOT(quit()));
    }

    virtual void run();

private:
    Job *q;
};

void Job::Private::run()
{
    q->doWork();

    if (q->isRunning()) {
        exec();
    }

#ifdef DEBUG_RACECONDITION
    QThread::sleep(2);
#endif
}

Job::Job(ReadOnlyArchiveInterface *interface, QObject *parent)
    : KJob(parent)
    , m_archiveInterface(interface)
    , m_isRunning(false)
    , d(new Private(this))
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
    if (d->isRunning()) {
        d->wait();
    }

    delete d;
}

ReadOnlyArchiveInterface *Job::archiveInterface()
{
    return m_archiveInterface;
}

bool Job::isRunning() const
{
    return m_isRunning;
}

void Job::start()
{
    m_isRunning = true;
    d->start();
}

void Job::emitResult()
{
    m_isRunning = false;
    KJob::emitResult();
}

void Job::connectToArchiveInterfaceSignals()
{
    connect(archiveInterface(), SIGNAL(error(QString,QString)), SLOT(onError(QString,QString)));
    connect(archiveInterface(), SIGNAL(entry(ArchiveEntry)), SLOT(onEntry(ArchiveEntry)));
    connect(archiveInterface(), SIGNAL(entryRemoved(QString)), SLOT(onEntryRemoved(QString)));
    connect(archiveInterface(), SIGNAL(progress(double)), SLOT(onProgress(double)));
    connect(archiveInterface(), SIGNAL(info(QString)), SLOT(onInfo(QString)));
    connect(archiveInterface(), SIGNAL(finished(bool)), SLOT(onFinished(bool)), Qt::DirectConnection);
    connect(archiveInterface(), SIGNAL(userQuery(Query*)), SLOT(onUserQuery(Query*)));
}

void Job::onError(const QString & message, const QString & details)
{
    Q_UNUSED(details)

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
    kDebug() << result;

    archiveInterface()->disconnect(this);

    emitResult();
}

void Job::onUserQuery(Query *query)
{
    emit userQuery(query);
}

bool Job::doKill()
{
    kDebug();
    bool ret = archiveInterface()->doKill();
    if (!ret) {
        kDebug() << "Killing does not seem to be supported here.";
    }
    return ret;
}

ListJob::ListJob(ReadOnlyArchiveInterface *interface, QObject *parent)
    : Job(interface, parent)
    , m_isSingleFolderArchive(true)
    , m_isPasswordProtected(false)
    , m_extractedFilesSize(0)
{
    connect(this, SIGNAL(newEntry(ArchiveEntry)),
            this, SLOT(onNewEntry(ArchiveEntry)));
}

void ListJob::doWork()
{
    emit description(this, i18n("Loading archive..."));
    connectToArchiveInterfaceSignals();
    bool ret = archiveInterface()->list();

    if (!archiveInterface()->waitForFinishedSignal()) {
        onFinished(ret);
    }
}

qlonglong ListJob::extractedFilesSize() const
{
    return m_extractedFilesSize;
}

bool ListJob::isPasswordProtected() const
{
    return m_isPasswordProtected;
}

bool ListJob::isSingleFolderArchive() const
{
    return m_isSingleFolderArchive;
}

void ListJob::onNewEntry(const ArchiveEntry& entry)
{
    m_extractedFilesSize += entry[ Size ].toLongLong();
    m_isPasswordProtected |= entry [ IsPasswordProtected ].toBool();

    if (m_isSingleFolderArchive) {
        const QString fileName(entry[FileName].toString());
        const QString basePath(fileName.split(QLatin1Char( '/' )).at(0));

        if (m_basePath.isEmpty()) {
            m_basePath = basePath;
            m_subfolderName = basePath;
        } else {
            if (m_basePath != basePath) {
                m_isSingleFolderArchive = false;
                m_subfolderName.clear();
            }
        }
    }
}

QString ListJob::subfolderName() const
{
    return m_subfolderName;
}

ExtractJob::ExtractJob(const QVariantList& files, const QString& destinationDir, ExtractionOptions options, ReadOnlyArchiveInterface *interface, QObject *parent)
    : Job(interface, parent)
    , m_files(files)
    , m_destinationDir(destinationDir)
    , m_options(options)
{
    setDefaultOptions();
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

    connectToArchiveInterfaceSignals();

    kDebug() << "Starting extraction with selected files:"
             << m_files
             << "Destination dir:" << m_destinationDir
             << "Options:" << m_options;

    bool ret = archiveInterface()->copyFiles(m_files, m_destinationDir, m_options);

    if (!archiveInterface()->waitForFinishedSignal()) {
        onFinished(ret);
    }
}

void ExtractJob::setDefaultOptions()
{
    ExtractionOptions defaultOptions;

    defaultOptions[QLatin1String("PreservePaths")] = false;

    ExtractionOptions::const_iterator it = defaultOptions.constBegin();
    for (; it != defaultOptions.constEnd(); ++it) {
        if (!m_options.contains(it.key())) {
            m_options[it.key()] = it.value();
        }
    }
}

QString ExtractJob::destinationDirectory() const
{
    return m_destinationDir;
}

ExtractionOptions ExtractJob::extractionOptions() const
{
    return m_options;
}

AddJob::AddJob(const QStringList& files, const CompressionOptions& options , ReadWriteArchiveInterface *interface, QObject *parent)
    : Job(interface, parent)
    , m_files(files)
    , m_options(options)
{
}

void AddJob::doWork()
{
    emit description(this, i18np("Adding a file", "Adding %1 files", m_files.count()));

    ReadWriteArchiveInterface *m_writeInterface =
        qobject_cast<ReadWriteArchiveInterface*>(archiveInterface());

    Q_ASSERT(m_writeInterface);

    connectToArchiveInterfaceSignals();
    bool ret = m_writeInterface->addFiles(m_files, m_options);

    if (!archiveInterface()->waitForFinishedSignal()) {
        onFinished(ret);
    }
}

DeleteJob::DeleteJob(const QVariantList& files, ReadWriteArchiveInterface *interface, QObject *parent)
    : Job(interface, parent)
    , m_files(files)
{
}

void DeleteJob::doWork()
{
    emit description(this, i18np("Deleting a file from the archive", "Deleting %1 files", m_files.count()));

    ReadWriteArchiveInterface *m_writeInterface =
        qobject_cast<ReadWriteArchiveInterface*>(archiveInterface());

    Q_ASSERT(m_writeInterface);

    connectToArchiveInterfaceSignals();
    int ret = m_writeInterface->deleteFiles(m_files);

    if (!archiveInterface()->waitForFinishedSignal()) {
        onFinished(ret);
    }
}

} // namespace Kerfuffle

#include "jobs.moc"

/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2009-2012 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "jobs.h"
#include "ark_debug.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QStorageInfo>
#include <QThread>
#include <QTimer>
#include <QUrl>

#include <KFileUtils>
#include <KLocalizedString>

namespace Kerfuffle
{

class Job::Private : public QThread
{
    Q_OBJECT

public:
    Private(Job *job, QObject *parent = nullptr)
        : QThread(parent)
        , q(job)
    {
    }

    void run() override;

private:
    Job *q;
};

void Job::Private::run()
{
    q->doWork();
}

Job::Job(Archive *archive, ReadOnlyArchiveInterface *interface)
    : KJob()
    , m_archive(archive)
    , m_archiveInterface(interface)
    , d(new Private(this))
{
    setCapabilities(KJob::Killable);
}

Job::Job(Archive *archive)
    : Job(archive, nullptr)
{}

Job::Job(ReadOnlyArchiveInterface *interface)
    : Job(nullptr, interface)
{}

Job::~Job()
{
    if (d->isRunning()) {
        d->wait();
    }

    delete d;
}

ReadOnlyArchiveInterface *Job::archiveInterface()
{
    // Use the archive interface.
    if (archive()) {
        return archive()->interface();
    }

    // Use the interface passed to this job (e.g. JSONArchiveInterface in jobstest.cpp).
    return m_archiveInterface;
}

Archive *Job::archive() const
{
    return m_archive;
}

QString Job::errorString() const
{
    if (!errorText().isEmpty()) {
        return errorText();
    }

    if (archive()) {
        if (archive()->error() == NoPlugin) {
            return i18n("No suitable plugin found. Ark does not seem to support this file type.");
        }

        if (archive()->error() == FailedPlugin) {
            return i18n("Failed to load a suitable plugin. Make sure any executables needed to handle the archive type are installed.");
        }
    }

    return QString();
}

void Job::start()
{
    jobTimer.start();

    // We have an archive but it's not valid, nothing to do.
    if (archive() && !archive()->isValid()) {
        QTimer::singleShot(0, this, [=]() {
            onFinished(false);
        });
        return;
    }

    if (archiveInterface()->waitForFinishedSignal()) {
        // CLI-based interfaces run a QProcess, no need to use threads.
        QTimer::singleShot(0, this, &Job::doWork);
    } else {
        // Run the job in another thread.
        d->start();
    }
}

void Job::connectToArchiveInterfaceSignals()
{
    connect(archiveInterface(), &ReadOnlyArchiveInterface::cancelled, this, &Job::onCancelled);
    connect(archiveInterface(), &ReadOnlyArchiveInterface::error, this, &Job::onError);
    connect(archiveInterface(), &ReadOnlyArchiveInterface::entry, this, &Job::onEntry);
    connect(archiveInterface(), &ReadOnlyArchiveInterface::progress, this, &Job::onProgress);
    connect(archiveInterface(), &ReadOnlyArchiveInterface::info, this, &Job::onInfo);
    connect(archiveInterface(), &ReadOnlyArchiveInterface::finished, this, &Job::onFinished);
    connect(archiveInterface(), &ReadOnlyArchiveInterface::userQuery, this, &Job::onUserQuery);

    auto readWriteInterface = qobject_cast<ReadWriteArchiveInterface*>(archiveInterface());
    if (readWriteInterface) {
        connect(readWriteInterface, &ReadWriteArchiveInterface::entryRemoved, this, &Job::onEntryRemoved);
    }
}

void Job::onCancelled()
{
    qCDebug(ARK) << "Cancelled emitted";
    setError(KJob::KilledJobError);
}

void Job::onError(const QString & message, const QString & details, int errorCode)
{
    Q_UNUSED(details)

    qCDebug(ARK) << "Error emitted:" << errorCode << "-" << message;
    setError(errorCode);
    setErrorText(message);
}

void Job::onEntry(Archive::Entry *entry)
{
    const QString entryFullPath = entry->fullPath();
    const QString cleanEntryFullPath = QDir::cleanPath(entryFullPath);
    if (cleanEntryFullPath.startsWith(QLatin1String("../")) || cleanEntryFullPath.contains(QLatin1String("/../"))) {
        qCWarning(ARK) << "Possibly malicious archive. Detected entry that could lead to a directory traversal attack:" << entryFullPath;
        onError(i18n("Could not load the archive because it contains ill-formed entries and might be a malicious archive."), QString(), Kerfuffle::PossiblyMaliciousArchiveError);
        onFinished(false);
        return;
    }

    Q_EMIT newEntry(entry);
}

void Job::onProgress(double value)
{
    setPercent(static_cast<unsigned long>(100.0*value));
}

void Job::onInfo(const QString& info)
{
    Q_EMIT infoMessage(this, info);
}

void Job::onEntryRemoved(const QString & path)
{
    Q_EMIT entryRemoved(path);
}

void Job::onFinished(bool result)
{
    qCDebug(ARK) << "Job finished, result:" << result << ", time:" << jobTimer.elapsed() << "ms";

    if (archive() && !archive()->isValid()) {
        setError(KJob::UserDefinedError);
    }

    if (!d->isInterruptionRequested()) {
        emitResult();
    }
}

void Job::onUserQuery(Query *query)
{
    if (archiveInterface()->waitForFinishedSignal()) {
        qCWarning(ARK) << "Plugins run from the main thread should call directly query->execute()";
    }

    Q_EMIT userQuery(query);
}

bool Job::doKill()
{
    const bool killed = archiveInterface()->doKill();
    if (killed) {
        return true;
    }

    if (d->isRunning()) {
        qCDebug(ARK) << "Requesting graceful thread interruption, will abort in one second otherwise.";
        d->requestInterruption();
        d->wait(1000);
    }

    return true;
}

LoadJob::LoadJob(Archive *archive, ReadOnlyArchiveInterface *interface)
    : Job(archive, interface)
    , m_isSingleFolderArchive(true)
    , m_isPasswordProtected(false)
    , m_extractedFilesSize(0)
    , m_dirCount(0)
    , m_filesCount(0)
{
    // Don't show "finished" notification when finished
    setProperty("transientProgressReporting", true);
    qCDebug(ARK) << "Created job instance";
    connect(this, &LoadJob::newEntry, this, &LoadJob::onNewEntry);
}

LoadJob::LoadJob(Archive *archive)
    : LoadJob(archive, nullptr)
{}

LoadJob::LoadJob(ReadOnlyArchiveInterface *interface)
    : LoadJob(nullptr, interface)
{}

void LoadJob::doWork()
{
    Q_EMIT description(this, i18n("Loading archive"), qMakePair(i18n("Archive"), archiveInterface()->filename()));
    connectToArchiveInterfaceSignals();

    bool ret = archiveInterface()->list();

    if (!archiveInterface()->waitForFinishedSignal()) {
        // onFinished() needs to be called after onNewEntry(), because the former reads members set in the latter.
        // So we need to put it in the event queue, just like the single-thread case does by emitting finished().
        QTimer::singleShot(0, this, [=]() {
            onFinished(ret);
        });
    }
}

void LoadJob::onFinished(bool result)
{
    if (archive() && result) {
        archive()->setProperty("unpackedSize", extractedFilesSize());
        archive()->setProperty("isSingleFolder", isSingleFolderArchive());
        const auto name = subfolderName().isEmpty() ? archive()->completeBaseName() : subfolderName();
        archive()->setProperty("subfolderName", name);
        if (isPasswordProtected()) {
            archive()->setProperty("encryptionType",  archive()->password().isEmpty() ? Archive::Encrypted : Archive::HeaderEncrypted);
        }
    }

    Job::onFinished(result);
}

qlonglong LoadJob::extractedFilesSize() const
{
    return m_extractedFilesSize;
}

bool LoadJob::isPasswordProtected() const
{
    return m_isPasswordProtected;
}

bool LoadJob::isSingleFolderArchive() const
{
    if (m_filesCount == 1 && m_dirCount == 0) {
        return false;
    }

    return m_isSingleFolderArchive;
}

void LoadJob::onNewEntry(const Archive::Entry *entry)
{
    m_extractedFilesSize += entry->isSparse() ? entry->sparseSize() : entry->property("size").toLongLong();
    m_isPasswordProtected |= entry->property("isPasswordProtected").toBool();

    if (entry->isDir()) {
        m_dirCount++;
    } else {
        m_filesCount++;
    }

    if (m_isSingleFolderArchive) {
        QString fullPath = entry->fullPath();
        // RPM filenames have the ./ prefix, and "." would be detected as the subfolder name, so we remove it.
        if (fullPath.startsWith(QLatin1String("./"))) {
            fullPath = fullPath.remove(0, 2);
        }

        const int index = fullPath.indexOf(QLatin1Char('/'));
        const QString basePath = fullPath.left(index);

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

QString LoadJob::subfolderName() const
{
    if (!isSingleFolderArchive()) {
        return QString();
    }

    return m_subfolderName;
}

BatchExtractJob::BatchExtractJob(LoadJob *loadJob, const QString &destination, bool autoSubfolder, bool preservePaths)
    : Job(loadJob->archive())
    , m_loadJob(loadJob)
    , m_destination(destination)
    , m_autoSubfolder(autoSubfolder)
    , m_preservePaths(preservePaths)
{
    qCDebug(ARK) << "Created job instance";
}

void BatchExtractJob::doWork()
{
    connect(m_loadJob, &KJob::result, this, &BatchExtractJob::slotLoadingFinished);
    connect(archiveInterface(), &ReadOnlyArchiveInterface::cancelled, this, &BatchExtractJob::onCancelled);

    if (archiveInterface()->hasBatchExtractionProgress()) {
        // progress() will be actually emitted by the LoadJob, but the archiveInterface() is the same.
        connect(archiveInterface(), &ReadOnlyArchiveInterface::progress, this, &BatchExtractJob::slotLoadingProgress);
    }

    // Forward LoadJob's signals.
    connect(m_loadJob, &Kerfuffle::Job::newEntry, this, &BatchExtractJob::newEntry);
    connect(m_loadJob, &Kerfuffle::Job::userQuery, this, &BatchExtractJob::userQuery);
    m_loadJob->start();
}

bool BatchExtractJob::doKill()
{
    if (m_step == Loading) {
        return m_loadJob->kill();
    }

    return m_extractJob->kill();
}

void BatchExtractJob::slotLoadingProgress(double progress)
{
    // Progress from LoadJob counts only for 50% of the BatchExtractJob's duration.
    m_lastPercentage = static_cast<unsigned long>(50.0*progress);
    setPercent(m_lastPercentage);
}

void BatchExtractJob::slotExtractProgress(double progress)
{
    // The 2nd 50% of the BatchExtractJob's duration comes from the ExtractJob.
    setPercent(m_lastPercentage + static_cast<unsigned long>(50.0*progress));
}

void BatchExtractJob::slotLoadingFinished(KJob *job)
{
    if (job->error()) {
        // Forward errors as well.
        onError(job->errorString(), QString(), job->error());
        onFinished(false);
        return;
    }

    // Block extraction if there's no space on the device.
    // Probably we need to take into account a small delta too,
    // so, free space + 1% just for the sake of it.
    QStorageInfo destinationStorage(m_destination);
    if (m_loadJob->extractedFilesSize() * 1.01 > destinationStorage.bytesAvailable()) {
        onError(xi18n("No space available on device <filename>%1</filename>", m_destination), QString(), Kerfuffle::DestinationNotWritableError);
        onFinished(false);
        return;
    }

    // Now we can start extraction.
    setupDestination();

    Kerfuffle::ExtractionOptions options;
    options.setPreservePaths(m_preservePaths);

    m_extractJob = archive()->extractFiles({}, m_destination, options);
    if (m_extractJob) {
        connect(m_extractJob, &KJob::result, this, &BatchExtractJob::emitResult);
        connect(m_extractJob, &Kerfuffle::Job::userQuery, this, &BatchExtractJob::userQuery);
        connect(archiveInterface(), &ReadOnlyArchiveInterface::error, this, &BatchExtractJob::onError);
        if (archiveInterface()->hasBatchExtractionProgress()) {
            // The LoadJob is done, change slot and start setting the percentage from m_lastPercentage on.
            disconnect(archiveInterface(), &ReadOnlyArchiveInterface::progress, this, &BatchExtractJob::slotLoadingProgress);
            connect(archiveInterface(), &ReadOnlyArchiveInterface::progress, this, &BatchExtractJob::slotExtractProgress);
        }
        m_step = Extracting;
        m_extractJob->start();
    } else {
        emitResult();
    }
}

void BatchExtractJob::setupDestination()
{
    const bool isSingleFolderRPM = (archive()->isSingleFolder() &&
                                   (archive()->mimeType().name() == QLatin1String("application/x-rpm")));

    if (m_autoSubfolder && (archive()->hasMultipleTopLevelEntries() || isSingleFolderRPM)) {
        const QDir d(m_destination);
        QString subfolderName = archive()->subfolderName();

        // Special case for single folder RPM archives.
        // We don't want the autodetected folder to have a meaningless "usr" name.
        if (isSingleFolderRPM && subfolderName == QLatin1String("usr")) {
            qCDebug(ARK) << "Detected single folder RPM archive. Using archive basename as subfolder name";
            subfolderName = QFileInfo(archive()->fileName()).completeBaseName();
        }

        if (d.exists(subfolderName)) {
            subfolderName = KFileUtils::suggestName(QUrl::fromUserInput(m_destination, QString(), QUrl::AssumeLocalFile), subfolderName);
        }

        d.mkdir(subfolderName);

        m_destination += QLatin1Char( '/' ) + subfolderName;
    }
}

CreateJob::CreateJob(Archive *archive, const QVector<Archive::Entry*> &entries, const CompressionOptions &options)
    : Job(archive)
    , m_entries(entries)
    , m_options(options)
{
    qCDebug(ARK) << "Created job instance";
    setTotalAmount(Files, 1);
}

CreateJob::~CreateJob()
{
    delete m_addJob;
}

void CreateJob::enableEncryption(const QString &password, bool encryptHeader)
{
    archive()->encrypt(password, encryptHeader);
}

void CreateJob::setMultiVolume(bool isMultiVolume)
{
    archive()->setMultiVolume(isMultiVolume);
}

void CreateJob::doWork()
{
    connect(archiveInterface(), &ReadOnlyArchiveInterface::progress, this, &CreateJob::onProgress);

    m_addJob = archive()->addFiles(m_entries, nullptr, m_options);

    if (m_addJob) {
        connect(m_addJob, &KJob::result, this, &CreateJob::emitResult);
        // Forward description signal from AddJob, we need to change the first
        // argument ('this' needs to be a CreateJob).
        connect(m_addJob, &KJob::description, this, [=](KJob *, const QString &title, const QPair<QString, QString> &field1, const QPair<QString, QString> &) {
            Q_EMIT description(this, title, field1);
        });

        m_addJob->start();
    } else {
        emitResult();
    }
}

bool CreateJob::doKill()
{
    bool killed = false;
    if (m_addJob) {
        killed = m_addJob->kill();

        if (killed) {
            // remove leftover archive if needed
            auto archiveFile = QFile(archive()->fileName());
            if (archiveFile.exists()) {
                archiveFile.remove();
            }
        }
    }

    return killed;
}

ExtractJob::ExtractJob(const QVector<Archive::Entry*> &entries, const QString &destinationDir, ExtractionOptions options, ReadOnlyArchiveInterface *interface)
    : Job(interface)
    , m_entries(entries)
    , m_destinationDir(destinationDir)
    , m_options(options)
{
    qCDebug(ARK) << "Created job instance";
    // Magic property that tells the job tracker the job's destination
    setProperty("destUrl", QUrl::fromLocalFile(destinationDir).toString());
}

void ExtractJob::doWork()
{
    const bool extractingAll = m_entries.empty();

    QString desc;
    if (extractingAll) {
        desc = i18n("Extracting all files");
    } else {
        desc = i18np("Extracting one file", "Extracting %1 files", m_entries.count());
    }
    Q_EMIT description(this, desc, qMakePair(i18n("Archive"), archiveInterface()->filename()), qMakePair(i18nc("extraction folder", "Destination"), m_destinationDir));

    QFileInfo destDirInfo(m_destinationDir);
    if (destDirInfo.isDir() && (!destDirInfo.isWritable() || !destDirInfo.isExecutable())) {
        onError(xi18n("Could not write to destination <filename>%1</filename>.<nl/>Check whether you have sufficient permissions.", m_destinationDir), QString(), Kerfuffle::DestinationNotWritableError);
        onFinished(false);
        return;
    }

    connectToArchiveInterfaceSignals();

    qCDebug(ARK) << "Starting extraction with" << m_entries.count() << "selected files."
             << m_entries
             << "Destination dir:" << m_destinationDir
             << "Options:" << m_options;

    qulonglong totalUncompressedSize = 0;
    if (extractingAll) {
        totalUncompressedSize = archiveInterface()->unpackedSize();
    } else {
        for (Archive::Entry *entry : qAsConst(m_entries)) {
            if (!entry->isDir()) {
                totalUncompressedSize += entry->isSparse() ? entry->sparseSize() : entry->size();
            }
        }
    }

    QStorageInfo destinationStorage(m_destinationDir);

    if (totalUncompressedSize > static_cast<qulonglong>(destinationStorage.bytesAvailable())) {
        onError(xi18n("No space available on device <filename>%1</filename>", m_destinationDir), QString(), Kerfuffle::DestinationNotWritableError);
        onFinished(false);
        return;
    }

    bool ret = archiveInterface()->extractFiles(m_entries, m_destinationDir, m_options);

    if (!archiveInterface()->waitForFinishedSignal()) {
        onFinished(ret);
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

TempExtractJob::TempExtractJob(Archive::Entry *entry, bool passwordProtectedHint, ReadOnlyArchiveInterface *interface)
    : Job(interface)
    , m_entry(entry)
    , m_passwordProtectedHint(passwordProtectedHint)
{
    m_tmpExtractDir = new QTemporaryDir();
}

Archive::Entry *TempExtractJob::entry() const
{
    return m_entry;
}

QString TempExtractJob::validatedFilePath() const
{
    QString path;
    // For single-file archives the filepath of the extracted entry is the displayName and not the fullpath.
    // TODO: find a better way to handle this.
    // Should the ReadOnlyArchiveInterface tell us which is the actual filepath of the entry that it has extracted?
    if (m_entry->displayName() != m_entry->name()) {
        path = extractionDir() + QLatin1Char('/') + m_entry->displayName();
    } else {
        path = extractionDir() + QLatin1Char('/') + m_entry->fullPath();
    }

    // Make sure a maliciously crafted archive with parent folders named ".." do
    // not cause the previewed file path to be located outside the temporary
    // directory, resulting in a directory traversal issue.
    path.remove(QStringLiteral("../"));

    return path;
}

ExtractionOptions TempExtractJob::extractionOptions() const
{
    ExtractionOptions options;

    if (m_passwordProtectedHint) {
        options.setEncryptedArchiveHint(true);
    }

    return options;
}

QTemporaryDir *TempExtractJob::tempDir() const
{
    return m_tmpExtractDir;
}

void TempExtractJob::doWork()
{
    // pass 1 to i18np on purpose so this translation may properly be reused.
    Q_EMIT description(this, i18np("Extracting one file", "Extracting %1 files", 1));

    connectToArchiveInterfaceSignals();

    qCDebug(ARK) << "Extracting:" << m_entry;

    bool ret = archiveInterface()->extractFiles({m_entry}, extractionDir(), extractionOptions());

    if (!archiveInterface()->waitForFinishedSignal()) {
        onFinished(ret);
    }
}

QString TempExtractJob::extractionDir() const
{
    return m_tmpExtractDir->path();
}

PreviewJob::PreviewJob(Archive::Entry *entry, bool passwordProtectedHint, ReadOnlyArchiveInterface *interface)
    : TempExtractJob(entry, passwordProtectedHint, interface)
{
    qCDebug(ARK) << "Created job instance";
}

OpenJob::OpenJob(Archive::Entry *entry, bool passwordProtectedHint, ReadOnlyArchiveInterface *interface)
    : TempExtractJob(entry, passwordProtectedHint, interface)
{
    qCDebug(ARK) << "Created job instance";
}

OpenWithJob::OpenWithJob(Archive::Entry *entry, bool passwordProtectedHint, ReadOnlyArchiveInterface *interface)
    : OpenJob(entry, passwordProtectedHint, interface)
{
    qCDebug(ARK) << "Created job instance";
}

AddJob::AddJob(const QVector<Archive::Entry*> &entries, const Archive::Entry *destination, const CompressionOptions& options, ReadWriteArchiveInterface *interface)
    : Job(interface)
    , m_entries(entries)
    , m_destination(destination)
    , m_options(options)
{
    qCDebug(ARK) << "Created job instance";
}

void AddJob::doWork()
{
    // Set current dir.
    const QString globalWorkDir = m_options.globalWorkDir();
    const QDir workDir = globalWorkDir.isEmpty() ? QDir::current() : QDir(globalWorkDir);
    if (!globalWorkDir.isEmpty()) {
        qCDebug(ARK) << "GlobalWorkDir is set, changing dir to " << globalWorkDir;
        m_oldWorkingDir = QDir::currentPath();
        QDir::setCurrent(globalWorkDir);
    }

    // Count total number of entries to be added.
    uint totalCount = 0;
    QElapsedTimer timer;
    timer.start();
    for (const Archive::Entry* entry : std::as_const(m_entries)) {
        totalCount++;
        if (QFileInfo(entry->fullPath()).isDir()) {
            QDirIterator it(entry->fullPath(), QDir::AllEntries | QDir::Readable | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                it.next();
                totalCount++;
            }
        }
    }

    qCDebug(ARK) << "Going to add" << totalCount << "entries, counted in" << timer.elapsed() << "ms";

    const QString desc = i18np("Compressing a file", "Compressing %1 files", totalCount);
    Q_EMIT description(this, desc, qMakePair(i18n("Archive"), archiveInterface()->filename()));

    ReadWriteArchiveInterface *m_writeInterface =
        qobject_cast<ReadWriteArchiveInterface*>(archiveInterface());

    Q_ASSERT(m_writeInterface);

    // The file paths must be relative to GlobalWorkDir.
    for (Archive::Entry *entry : std::as_const(m_entries)) {
        // #191821: workDir must be used instead of QDir::current()
        //          so that symlinks aren't resolved automatically
        const QString &fullPath = entry->fullPath();
        QString relativePath = workDir.relativeFilePath(fullPath);

        if (fullPath.endsWith(QLatin1Char('/'))) {
            relativePath += QLatin1Char('/');
        }

        entry->setFullPath(relativePath);
    }

    connectToArchiveInterfaceSignals();
    bool ret = m_writeInterface->addFiles(m_entries, m_destination, m_options, totalCount);

    if (!archiveInterface()->waitForFinishedSignal()) {
        onFinished(ret);
    }
}

void AddJob::onFinished(bool result)
{
    if (!m_oldWorkingDir.isEmpty()) {
        QDir::setCurrent(m_oldWorkingDir);
    }

    Job::onFinished(result);
}

MoveJob::MoveJob(const QVector<Archive::Entry*> &entries, Archive::Entry *destination, const CompressionOptions& options , ReadWriteArchiveInterface *interface)
    : Job(interface)
    , m_finishedSignalsCount(0)
    , m_entries(entries)
    , m_destination(destination)
    , m_options(options)
{
    qCDebug(ARK) << "Created job instance";
}

void MoveJob::doWork()
{
    qCDebug(ARK) << "Going to move" << m_entries.count() << "file(s)";

    QString desc = i18np("Moving a file", "Moving %1 files", m_entries.count());
    Q_EMIT description(this, desc, qMakePair(i18n("Archive"), archiveInterface()->filename()));

    ReadWriteArchiveInterface *m_writeInterface =
        qobject_cast<ReadWriteArchiveInterface*>(archiveInterface());

    Q_ASSERT(m_writeInterface);

    connectToArchiveInterfaceSignals();
    bool ret = m_writeInterface->moveFiles(m_entries, m_destination, m_options);

    if (!archiveInterface()->waitForFinishedSignal()) {
        onFinished(ret);
    }
}

void MoveJob::onFinished(bool result)
{
    m_finishedSignalsCount++;
    if (m_finishedSignalsCount == archiveInterface()->moveRequiredSignals()) {
        Job::onFinished(result);
    }
}

CopyJob::CopyJob(const QVector<Archive::Entry*> &entries, Archive::Entry *destination, const CompressionOptions &options, ReadWriteArchiveInterface *interface)
    : Job(interface)
    , m_finishedSignalsCount(0)
    , m_entries(entries)
    , m_destination(destination)
    , m_options(options)
{
    qCDebug(ARK) << "Created job instance";
}

void CopyJob::doWork()
{
    qCDebug(ARK) << "Going to copy" << m_entries.count() << "file(s)";

    QString desc = i18np("Copying a file", "Copying %1 files", m_entries.count());
    Q_EMIT description(this, desc, qMakePair(i18n("Archive"), archiveInterface()->filename()));

    ReadWriteArchiveInterface *m_writeInterface =
        qobject_cast<ReadWriteArchiveInterface*>(archiveInterface());

    Q_ASSERT(m_writeInterface);

    connectToArchiveInterfaceSignals();
    bool ret = m_writeInterface->copyFiles(m_entries, m_destination, m_options);

    if (!archiveInterface()->waitForFinishedSignal()) {
        onFinished(ret);
    }
}

void CopyJob::onFinished(bool result)
{
    m_finishedSignalsCount++;
    if (m_finishedSignalsCount == archiveInterface()->copyRequiredSignals()) {
        Job::onFinished(result);
    }
}

DeleteJob::DeleteJob(const QVector<Archive::Entry*> &entries, ReadWriteArchiveInterface *interface)
    : Job(interface)
    , m_entries(entries)
{
}

void DeleteJob::doWork()
{
    QString desc = i18np("Deleting a file from the archive", "Deleting %1 files", m_entries.count());
    Q_EMIT description(this, desc, qMakePair(i18n("Archive"), archiveInterface()->filename()));

    ReadWriteArchiveInterface *m_writeInterface =
        qobject_cast<ReadWriteArchiveInterface*>(archiveInterface());

    Q_ASSERT(m_writeInterface);

    connectToArchiveInterfaceSignals();
    bool ret = m_writeInterface->deleteFiles(m_entries);

    if (!archiveInterface()->waitForFinishedSignal()) {
        onFinished(ret);
    }
}

CommentJob::CommentJob(const QString& comment, ReadWriteArchiveInterface *interface)
    : Job(interface)
    , m_comment(comment)
{
}

void CommentJob::doWork()
{
    Q_EMIT description(this, i18n("Adding comment"));

    ReadWriteArchiveInterface *m_writeInterface =
        qobject_cast<ReadWriteArchiveInterface*>(archiveInterface());

    Q_ASSERT(m_writeInterface);

    connectToArchiveInterfaceSignals();
    bool ret = m_writeInterface->addComment(m_comment);

    if (!archiveInterface()->waitForFinishedSignal()) {
        onFinished(ret);
    }
}

TestJob::TestJob(ReadOnlyArchiveInterface *interface)
    : Job(interface)
{
    m_testSuccess = false;
}

void TestJob::doWork()
{
    qCDebug(ARK) << "Job started";

    Q_EMIT description(this, i18n("Testing archive"), qMakePair(i18n("Archive"), archiveInterface()->filename()));

    connectToArchiveInterfaceSignals();
    connect(archiveInterface(), &ReadOnlyArchiveInterface::testSuccess, this, &TestJob::onTestSuccess);

    bool ret = archiveInterface()->testArchive();

    if (!archiveInterface()->waitForFinishedSignal()) {
        onFinished(ret);
    }
}

void TestJob::onTestSuccess()
{
    m_testSuccess = true;
}

bool TestJob::testSucceeded()
{
    return m_testSuccess;
}

} // namespace Kerfuffle

#include "jobs.moc"

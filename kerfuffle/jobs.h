/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (c) 2008-2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (c) 2009-2012 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (c) 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
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

#ifndef JOBS_H
#define JOBS_H

#include "kerfuffle_export.h"
#include "archiveinterface.h"
#include "archive_kerfuffle.h"
#include "archiveentry.h"
#include "queries.h"

#include <KJob>

#include <QElapsedTimer>
#include <QTemporaryDir>
#include <QPointer>

namespace Kerfuffle
{

class KERFUFFLE_EXPORT Job : public KJob
{
    Q_OBJECT

public:

    /**
     * @return The archive processed by this job.
     * @warning This method should not be called before start().
     */
    Archive *archive() const;
    QString errorString() const override;
    void start() override;

protected:
    Job(Archive *archive, ReadOnlyArchiveInterface *interface);
    Job(Archive *archive);
    Job(ReadOnlyArchiveInterface *interface);
    ~Job() override;
    bool doKill() override;

    ReadOnlyArchiveInterface *archiveInterface();

    void connectToArchiveInterfaceSignals();

public Q_SLOTS:
    virtual void doWork() = 0;

protected Q_SLOTS:
    virtual void onCancelled();
    virtual void onError(const QString &message, const QString &details, int errorCode);
    virtual void onInfo(const QString &info);
    virtual void onEntry(Archive::Entry *entry);
    virtual void onProgress(double progress);
    virtual void onEntryRemoved(const QString &path);
    virtual void onFinished(bool result);
    virtual void onUserQuery(Kerfuffle::Query *query);

Q_SIGNALS:
    void entryRemoved(const QString & entry);
    void newEntry(Archive::Entry*);
    void userQuery(Kerfuffle::Query*);

private:
    Archive *m_archive;
    ReadOnlyArchiveInterface *m_archiveInterface;
    QElapsedTimer jobTimer;

    class Private;
    Private * const d;
};

/**
 * Load an existing archive.
 * Example usage:
 *
 * \code
 *
 * auto job = Archive::load(filename);
 * connect(job, &KJob::result, [](KJob *job) {
 *     if (!job->error) {
 *         auto archive = qobject_cast<Archive::LoadJob*>(job)->archive();
 *         // do something with archive.
 *     }
 * });
 * job->start();
 *
 * \endcode
 */
class KERFUFFLE_EXPORT LoadJob : public Job
{
    Q_OBJECT

public:
    explicit LoadJob(Archive *archive);
    explicit LoadJob(ReadOnlyArchiveInterface *interface);

    qlonglong extractedFilesSize() const;
    bool isPasswordProtected() const;
    bool isSingleFolderArchive() const;
    QString subfolderName() const;

public Q_SLOTS:
    void doWork() override;

protected Q_SLOTS:
    void onFinished(bool result) override;

private:
    explicit LoadJob(Archive *archive, ReadOnlyArchiveInterface *interface);

    bool m_isSingleFolderArchive;
    bool m_isPasswordProtected;
    QString m_subfolderName;
    QString m_basePath;
    qlonglong m_extractedFilesSize;
    qlonglong m_dirCount;
    qlonglong m_filesCount;

private Q_SLOTS:
    void onNewEntry(const Archive::Entry*);
};

/**
 * Perform a batch extraction of an existing archive.
 * Internally it runs a LoadJob before the actual extraction,
 * to figure out properties such as the subfolder name.
 */
class KERFUFFLE_EXPORT BatchExtractJob : public Job
{
    Q_OBJECT

public:
    explicit BatchExtractJob(LoadJob *loadJob, const QString &destination, bool autoSubfolder, bool preservePaths);

public Q_SLOTS:
    void doWork() override;

protected:
    bool doKill() override;

private Q_SLOTS:
    void slotLoadingProgress(double progress);
    void slotExtractProgress(double progress);
    void slotLoadingFinished(KJob *job);

private:

    /**
     * Tracks whether the job is loading or extracting the archive.
     */
    enum Step {Loading, Extracting};

    void setupDestination();

    Step m_step = Loading;
    ExtractJob *m_extractJob = nullptr;
    LoadJob *m_loadJob;
    QString m_destination;
    bool m_autoSubfolder;
    bool m_preservePaths;
    unsigned long m_lastPercentage = 0;
};

/**
 * Create a new archive given a bunch of entries.
 */
class KERFUFFLE_EXPORT CreateJob : public Job
{
    Q_OBJECT

public:
    explicit CreateJob(Archive *archive, const QVector<Archive::Entry*> &entries, const CompressionOptions& options);

    /**
     * @param password The password to encrypt the archive with.
     * @param encryptHeader Whether to encrypt also the list of files.
     */
    void enableEncryption(const QString &password, bool encryptHeader);

    /**
     * Set whether the new archive should be multivolume.
     */
    void setMultiVolume(bool isMultiVolume);

public Q_SLOTS:
    void doWork() override;

protected:
    bool doKill() override;

private:
    QPointer<AddJob> m_addJob;
    QVector<Archive::Entry*> m_entries;
    CompressionOptions m_options;
};

class KERFUFFLE_EXPORT ExtractJob : public Job
{
    Q_OBJECT

public:
    ExtractJob(const QVector<Archive::Entry*> &entries, const QString& destinationDir, ExtractionOptions options, ReadOnlyArchiveInterface *interface);

    QString destinationDirectory() const;
    ExtractionOptions extractionOptions() const;

public Q_SLOTS:
    void doWork() override;

private:

    QVector<Archive::Entry*> m_entries;
    QString m_destinationDir;
    ExtractionOptions m_options;
};

/**
 * Abstract base class for jobs that extract a single file to a temporary dir.
 * It's not possible to pass extraction options and paths will be always preserved.
 * The only option that the job needs to know is whether the file is password protected.
 */
class KERFUFFLE_EXPORT TempExtractJob : public Job
{
    Q_OBJECT

public:
    TempExtractJob(Archive::Entry *entry, bool passwordProtectedHint, ReadOnlyArchiveInterface *interface);

    /**
     * @return The absolute path of the extracted file.
     * The path is validated in order to prevent directory traversal attacks.
     */
    QString validatedFilePath() const;

    ExtractionOptions extractionOptions() const;

    /**
     * @return The temporary dir used for the extraction.
     * It is safe to delete this pointer in order to remove the directory.
     */
    QTemporaryDir *tempDir() const;

public Q_SLOTS:
    void doWork() override;

private:
    QString extractionDir() const;

    Archive::Entry *m_entry;
    QTemporaryDir *m_tmpExtractDir;
    bool m_passwordProtectedHint;
};

/**
 * This TempExtractJob can be used to preview a file.
 * The temporary extraction directory will be deleted upon job's completion.
 */
class KERFUFFLE_EXPORT PreviewJob : public TempExtractJob
{
    Q_OBJECT

public:
    PreviewJob(Archive::Entry *entry, bool passwordProtectedHint, ReadOnlyArchiveInterface *interface);
};

/**
 * This TempExtractJob can be used to open a file in its dedicated application.
 * For this reason, the temporary extraction directory will NOT be deleted upon job's completion.
 */
class KERFUFFLE_EXPORT OpenJob : public TempExtractJob
{
    Q_OBJECT

public:
    OpenJob(Archive::Entry *entry, bool passwordProtectedHint, ReadOnlyArchiveInterface *interface);
};

class KERFUFFLE_EXPORT OpenWithJob : public OpenJob
{
    Q_OBJECT

public:
    OpenWithJob(Archive::Entry *entry, bool passwordProtectedHint, ReadOnlyArchiveInterface *interface);
};

class KERFUFFLE_EXPORT AddJob : public Job
{
    Q_OBJECT

public:
    AddJob(const QVector<Archive::Entry*> &files, const Archive::Entry *destination, const CompressionOptions& options, ReadWriteArchiveInterface *interface);

public Q_SLOTS:
    void doWork() override;

protected Q_SLOTS:
    void onFinished(bool result) override;

private:
    QString m_oldWorkingDir;
    const QVector<Archive::Entry*> m_entries;
    const Archive::Entry *m_destination;
    CompressionOptions m_options;
};

/**
 * This MoveJob can be used to rename or move entries within the archive.
 * @see Archive::moveFiles for more details.
 */
class KERFUFFLE_EXPORT MoveJob : public Job
{
Q_OBJECT

public:
    MoveJob(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions& options, ReadWriteArchiveInterface *interface);

public Q_SLOTS:
    void doWork() override;

protected Q_SLOTS:
    void onFinished(bool result) override;

private:
    int m_finishedSignalsCount;
    const QVector<Archive::Entry*> m_entries;
    Archive::Entry *m_destination;
    CompressionOptions m_options;
};

/**
 * This CopyJob can be used to copy entries within the archive.
 * @see Archive::copyFiles for more details.
 */
class KERFUFFLE_EXPORT CopyJob : public Job
{
Q_OBJECT

public:
    CopyJob(const QVector<Archive::Entry*> &entries, Archive::Entry *destination, const CompressionOptions& options, ReadWriteArchiveInterface *interface);

public Q_SLOTS:
    void doWork() override;

protected Q_SLOTS:
    void onFinished(bool result) override;

private:
    int m_finishedSignalsCount;
    const QVector<Archive::Entry*> m_entries;
    Archive::Entry *m_destination;
    CompressionOptions m_options;
};

class KERFUFFLE_EXPORT DeleteJob : public Job
{
    Q_OBJECT

public:
    DeleteJob(const QVector<Archive::Entry*> &files, ReadWriteArchiveInterface *interface);

public Q_SLOTS:
    void doWork() override;

private:
    QVector<Archive::Entry*> m_entries;
};

class KERFUFFLE_EXPORT CommentJob : public Job
{
    Q_OBJECT

public:
    CommentJob(const QString& comment, ReadWriteArchiveInterface *interface);

public Q_SLOTS:
    void doWork() override;

private:
    QString m_comment;
};

class KERFUFFLE_EXPORT TestJob : public Job
{
    Q_OBJECT

public:
    explicit TestJob(ReadOnlyArchiveInterface *interface);
    bool testSucceeded();

public Q_SLOTS:
    void doWork() override;

private Q_SLOTS:
    virtual void onTestSuccess();

private:
    bool m_testSuccess;

};

} // namespace Kerfuffle

#endif // JOBS_H

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

#ifndef JOBS_H
#define JOBS_H

#include "kerfuffle_export.h"
#include "archiveinterface.h"
#include "archive.h"
#include "queries.h"

#include <KJob>
#include <QList>
#include <QVariant>
#include <QString>

namespace Kerfuffle
{

class ThreadExecution;

class KERFUFFLE_EXPORT Job : public KJob
{
    Q_OBJECT

public:
    void start();

    bool isRunning() const;

protected:
    Job(ReadOnlyArchiveInterface *interface, QObject *parent = 0);
    virtual ~Job();
    virtual bool doKill();
    virtual void emitResult();

    ReadOnlyArchiveInterface *archiveInterface();

    void connectToArchiveInterfaceSignals();

public slots:
    virtual void doWork() = 0;

protected slots:
    virtual void onError(const QString &message, const QString &details);
    virtual void onInfo(const QString &info);
    virtual void onEntry(const ArchiveEntry &archiveEntry);
    virtual void onProgress(double progress);
    virtual void onEntryRemoved(const QString &path);
    virtual void onFinished(bool result);
    virtual void onUserQuery(Query *query);

signals:
    void entryRemoved(const QString & entry);
    void error(const QString& errorMessage, const QString& details);
    void newEntry(const ArchiveEntry &);
    void userQuery(Kerfuffle::Query*);

private:
    ReadOnlyArchiveInterface *m_archiveInterface;

    bool m_isRunning;

    class Private;
    Private * const d;
};

class KERFUFFLE_EXPORT ListJob : public Job
{
    Q_OBJECT

public:
    explicit ListJob(ReadOnlyArchiveInterface *interface, QObject *parent = 0);

    qlonglong extractedFilesSize() const;
    bool isPasswordProtected() const;
    bool isSingleFolderArchive() const;
    QString subfolderName() const;

public slots:
    virtual void doWork();

private:
    bool m_isSingleFolderArchive;
    bool m_isPasswordProtected;
    QString m_subfolderName;
    QString m_basePath;
    qlonglong m_extractedFilesSize;

private slots:
    void onNewEntry(const ArchiveEntry&);
};

class KERFUFFLE_EXPORT ExtractJob : public Job
{
    Q_OBJECT

public:
    ExtractJob(const QVariantList& files, const QString& destinationDir, ExtractionOptions options, ReadOnlyArchiveInterface *interface, QObject *parent = 0);

    QString destinationDirectory() const;
    ExtractionOptions extractionOptions() const;

public slots:
    virtual void doWork();

private:
    // TODO: Maybe this should be a method if ExtractionOptions were a class?
    void setDefaultOptions();

    QVariantList m_files;
    QString m_destinationDir;
    ExtractionOptions m_options;
};

class KERFUFFLE_EXPORT AddJob : public Job
{
    Q_OBJECT

public:
    AddJob(const QStringList& files, const CompressionOptions& options, ReadWriteArchiveInterface *interface, QObject *parent = 0);

public slots:
    virtual void doWork();

private:
    QStringList m_files;
    CompressionOptions m_options;
};

class KERFUFFLE_EXPORT DeleteJob : public Job
{
    Q_OBJECT

public:
    DeleteJob(const QVariantList& files, ReadWriteArchiveInterface *interface, QObject *parent = 0);

public slots:
    virtual void doWork();

private:
    QVariantList m_files;
};

} // namespace Kerfuffle

#endif // JOBS_H

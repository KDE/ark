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
#ifndef JOBS_H
#define JOBS_H

#include "kerfuffle_export.h"
#include "archiveinterface.h"
#include "archive.h"
#include "queries.h"
#include "observer.h"

#include <KJob>
#include <QList>
#include <QVariant>
#include <QString>

namespace Kerfuffle
{
class ThreadExecution;

class KERFUFFLE_EXPORT Job: public KJob, public ArchiveObserver
{
    Q_OBJECT

public:
    void start();

    //abstract implemented methods from observer
    virtual void onError(const QString & message, const QString & details);
    virtual void onInfo(const QString & info);
    virtual void onEntry(const ArchiveEntry & archiveEntry);
    virtual void onProgress(double);
    virtual void onEntryRemoved(const QString & path);
    virtual void onFinished(bool result);
    virtual void onUserQuery(class Query *query);

protected:
    Job(ReadOnlyArchiveInterface *interface, QObject *parent = 0);
    virtual ~Job();
    virtual bool doKill();

    ReadOnlyArchiveInterface *m_interface;

private:
    ThreadExecution *m_workerThread;

public slots:
    virtual void doWork() = 0;

signals:
    void entryRemoved(const QString & entry);
    void error(const QString& errorMessage, const QString& details);
    void newEntry(const ArchiveEntry &);
    void userQuery(Query*);
};

class KERFUFFLE_EXPORT ListJob: public Job
{
    Q_OBJECT

public:
    explicit ListJob(ReadOnlyArchiveInterface *interface, QObject *parent = 0);

    void doWork();
    bool isSingleFolderArchive() {
        return m_isSingleFolderArchive;
    }
    bool isPasswordProtected() {
        return m_isPasswordProtected;
    }
    QString subfolderName() {
        return m_subfolderName;
    }
    qlonglong extractedFilesSize() {
        return m_extractedFilesSize;
    }

private:
    bool m_isSingleFolderArchive;
    bool m_isPasswordProtected;
    QString m_subfolderName;
    QString m_previousEntry;
    qlonglong m_extractedFilesSize;

private slots:
    void onNewEntry(const ArchiveEntry&);
};

class KERFUFFLE_EXPORT ExtractJob: public Job
{
    Q_OBJECT

public:
    ExtractJob(const QList<QVariant> & files, const QString&
               destinationDir, ExtractionOptions options,
               ReadOnlyArchiveInterface *interface, QObject *parent = 0);

    void doWork();

private:
    void fillInDefaultValues(ExtractionOptions& options);
    QList<QVariant> m_files;
    QString m_destinationDir;
    ExtractionOptions m_options;
};

class KERFUFFLE_EXPORT AddJob: public Job
{
    Q_OBJECT

public:
    AddJob(const QStringList & files, const CompressionOptions& options,
           ReadWriteArchiveInterface *interface, QObject *parent = 0
          );
    void doWork();

private:
    QStringList m_files;
    CompressionOptions m_options;

};

class KERFUFFLE_EXPORT DeleteJob: public Job
{
    Q_OBJECT

public:
    DeleteJob(const QList<QVariant>& files, ReadWriteArchiveInterface
              *interface, QObject *parent = 0);
    void doWork();

private:
    QList<QVariant> m_files;
};
} // namespace Kerfuffle

#endif // JOBS_H

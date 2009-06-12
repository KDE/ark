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

#ifndef ARCHIVEINTERFACE_H
#define ARCHIVEINTERFACE_H

#include <QObject>
#include <QStringList>
#include <QString>

#include "archive.h"
#include "queries.h"
#include "kerfuffle_export.h"

#include <kio/job.h>
#include <kio/jobclasses.h>
#include <kfileitem.h>

namespace Kerfuffle
{
class ArchiveObserver;

class KERFUFFLE_EXPORT ReadOnlyArchiveInterface: public QObject
{
    Q_OBJECT
public:
    explicit ReadOnlyArchiveInterface(const QString & filename, QObject *parent = 0);
    virtual ~ReadOnlyArchiveInterface();

    /** Return the filename of currently handled archive. */
    const QString& filename() const;
    virtual bool isReadOnly() const;

    void KDE_NO_EXPORT registerObserver(ArchiveObserver *observer);
    void KDE_NO_EXPORT removeObserver(ArchiveObserver *observer);

    virtual bool open();

    /**
     * List archive contents.
     * This runs the process of reading archive contents.
     * When subclassing, you can block as long as you need, the function runs
     * in its own thread.
     * @returns whether the listing succeeded.
     * @note If returning false, make sure to call error() beforewards to notify
     * the user of the error condition.
     */
    virtual bool list() = 0;
    void setPassword(QString password);

    /**
     * Extract files from archive.
     * Globally recognized extraction options:
     * @li PreservePaths - preserve file paths (extract flat if false)
     * @li RootNode - node in the archive which will correspond to the @arg destinationDirectory
     * When subclassing, you can block as long as you need, the function runs
     * in its own thread.
     * @returns whether the listing succeeded.
     * @note If returning false, make sure to call error() beforewards to notify
     * the user of the error condition.
     */
    virtual bool copyFiles(const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options) = 0;

    bool waitForFinishedSignal() {
        return m_waitForFinishedSignal;
    }
    void finished(bool result);

    virtual bool doKill();
    virtual bool doSuspend();
    virtual bool doResume();

protected:
    /**
     * Communicate an error.
     * Sets message of error for user to read and understand. It will be
     * displayed once the job has returned false.
     */
    void error(const QString & message, const QString & details = QString());

    /**
     * Notify observers of a new archive entry.
     * The interface should call this function whenever a new archive entry
     * is read. @note Remember that directories should have filename ending with /.
     */
    void entry(const ArchiveEntry & archiveEntry);
    void progress(double);
    void info(const QString& info);
    void entryRemoved(const QString& path);
    const QString& password() const;
    void userQuery(Query*);
    /**
     * Setting this option to true will not exit the tread with the
     * exit of the various functions, but rather when finished(bool) is
     * called. Doing this one can use the event loop easily while doing
     * the operation.
     */
    void setWaitForFinishedSignal(bool value);



private:
    QList<ArchiveObserver*> m_observers;
    QString m_filename;
    QString m_password;
    bool m_waitForFinishedSignal;
};

class KERFUFFLE_EXPORT ReadWriteArchiveInterface: public ReadOnlyArchiveInterface
{
    Q_OBJECT
public:
    explicit ReadWriteArchiveInterface(const QString & filename, QObject *parent = 0);
    virtual ~ReadWriteArchiveInterface();

    virtual bool isReadOnly() const;

    //see archive.h for a list of what the compressionoptions might
    //contain
    virtual bool addFiles(const QStringList & files, const CompressionOptions& options) = 0;
    virtual bool deleteFiles(const QList<QVariant> & files) = 0;
};

} // namespace Kerfuffle

#endif // ARCHIVEINTERFACE_H

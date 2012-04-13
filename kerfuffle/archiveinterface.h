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

#ifndef ARCHIVEINTERFACE_H
#define ARCHIVEINTERFACE_H

#include "archive.h"
#include "kerfuffle_export.h"

#include <QObject>
#include <QStringList>
#include <QString>
#include <QVariantList>

namespace Kerfuffle
{
class Query;

class KERFUFFLE_EXPORT ReadOnlyArchiveInterface: public QObject
{
    Q_OBJECT
public:
    explicit ReadOnlyArchiveInterface(QObject *parent, const QVariantList & args);
    virtual ~ReadOnlyArchiveInterface();

    /**
     * Returns the filename of the archive currently being handled.
     */
    QString filename() const;

    /**
     * Returns whether the file can only be read.
     *
     * @return @c true  The file cannot be written.
     * @return @c false The file can be read and written.
     */
    virtual bool isReadOnly() const;

    virtual bool open();

    /**
     * List archive contents.
     * This runs the process of reading archive contents.
     * When subclassing, you can block as long as you need, the function runs
     * in its own thread.
     * @returns whether the listing succeeded.
     * @note If returning false, make sure to emit the error() signal beforewards to notify
     * the user of the error condition.
     */
    virtual bool list() = 0;
    void setPassword(const QString &password);

    /**
     * Extract files from archive.
     * Globally recognized extraction options:
     * @li PreservePaths - preserve file paths (extract flat if false)
     * @li RootNode - node in the archive which will correspond to the @arg destinationDirectory
     * When subclassing, you can block as long as you need, the function runs
     * in its own thread.
     * @returns whether the listing succeeded.
     * @note If returning false, make sure to emit the error() signal beforewards to notify
     * the user of the error condition.
     */
    virtual bool copyFiles(const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options) = 0;

    bool waitForFinishedSignal();

    virtual bool doKill();
    virtual bool doSuspend();
    virtual bool doResume();

signals:
    void error(const QString &message, const QString &details = QString());
    void entry(const ArchiveEntry &archiveEntry);
    void entryRemoved(const QString &path);
    void progress(double progress);
    void info(const QString &info);
    void finished(bool result);
    void userQuery(Query *query);

protected:
    QString password() const;
    /**
     * Setting this option to true will not exit the thread with the
     * exit of the various functions, but rather when finished(bool) is
     * called. Doing this one can use the event loop easily while doing
     * the operation.
     */
    void setWaitForFinishedSignal(bool value);

private:
    QString m_filename;
    QString m_password;
    bool m_waitForFinishedSignal;
};

class KERFUFFLE_EXPORT ReadWriteArchiveInterface: public ReadOnlyArchiveInterface
{
    Q_OBJECT
public:
    explicit ReadWriteArchiveInterface(QObject *parent, const QVariantList & args);
    virtual ~ReadWriteArchiveInterface();

    virtual bool isReadOnly() const;

    //see archive.h for a list of what the compressionoptions might
    //contain
    virtual bool addFiles(const QStringList & files, const CompressionOptions& options) = 0;
    virtual bool deleteFiles(const QList<QVariant> & files) = 0;
};

} // namespace Kerfuffle

#endif // ARCHIVEINTERFACE_H

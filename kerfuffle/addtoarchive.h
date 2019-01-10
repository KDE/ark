/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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

#ifndef ADDTOARCHIVE_H
#define ADDTOARCHIVE_H

#include "kerfuffle_export.h"
#include "archive_kerfuffle.h"

#include <KJob>

#include <QUrl>
#include <QStringList>

/**
 * Compresses all input files into an archive.
 *
 * This is a job class that creates a compressed archive
 * with all the given input files.
 *
 * It provides the functionality for the --add command-line
 * option, and does not need the GUI to be running.
 *
 * @author Harald Hvaal <haraldhv@stud.ntnu.no>
 */
namespace Kerfuffle
{
class KERFUFFLE_EXPORT AddToArchive : public KJob
{
    Q_OBJECT

public:
    explicit AddToArchive(QObject *parent = nullptr);
    ~AddToArchive() override;

    bool showAddDialog();
    void setPreservePaths(bool value);
    void setChangeToFirstPath(bool value);
    QString detectBaseName(const QVector<Archive::Entry*> &entries) const;

public Q_SLOTS:
    bool addInput(const QUrl &url);
    void setAutoFilenameSuffix(const QString& suffix);
    void setFilename(const QUrl &path);
    void setMimeType(const QString & mimeType);
    void setPassword(const QString &password);
    void setHeaderEncryptionEnabled(bool enabled);
    void start() override;

protected:
    bool doKill() override;

private Q_SLOTS:
    void slotFinished(KJob*);
    void slotStartJob();

private:
    void detectFileName();

    CompressionOptions m_options;
    CreateJob *m_createJob;
    QString m_filename;
    QString m_strippedPath;
    QString m_autoFilenameSuffix;
    QString m_firstPath;
    QString m_mimeType;
    QString m_password;
    QVector<Archive::Entry*> m_entries;
    bool m_changeToFirstPath;
    bool m_enableHeaderEncryption;
};
}

#endif // ADDTOARCHIVE_H


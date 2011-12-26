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

#include <KJob>
#include <KUrl>

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
    AddToArchive(QObject *parent = 0);
    ~AddToArchive();

    bool showAddDialog();
    void setPreservePaths(bool value);
    void setChangeToFirstPath(bool value);

public slots:
    bool addInput(const KUrl& url);
    void setAutoFilenameSuffix(const QString& suffix);
    void setFilename(const KUrl& path);
    void setMimeType(const QString & mimeType);
    void start();

private slots:
    void slotFinished(KJob*);
    void slotStartJob();

private:
    QString m_filename;
    QString m_strippedPath;
    QString m_autoFilenameSuffix;
    QString m_firstPath;
    QString m_mimeType;
    QStringList m_inputs;
    bool m_changeToFirstPath;
};
}

#endif // ADDTOARCHIVE_H


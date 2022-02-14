/*
    SPDX-FileCopyrightText: 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef ADDTOARCHIVE_H
#define ADDTOARCHIVE_H

#include "kerfuffle_export.h"
#include "archive_kerfuffle.h"

#include <KJob>

#include <QUrl>

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
    static QString getFileName(const QList<QUrl> &entries);
    void setImmediateProgressReporting(bool immediateProgressReporting);

    QString fileName() const
    {
        return m_filename;
    }

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
    static QString detectBaseName(const QList<QUrl> &entries);
    static QString getBaseName(const QString &url, const int size);
    static QString getFileName(const QString &base, const QString &suffix);
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
    bool m_immediateProgressReporting;
};
}

#endif // ADDTOARCHIVE_H


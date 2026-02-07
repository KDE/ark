/*
    SPDX-FileCopyrightText: 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef ADDTOARCHIVE_H
#define ADDTOARCHIVE_H

#include "archive_kerfuffle.h"
#include "kerfuffle_export.h"

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

    bool showAddDialog(QWidget *parentWidget);
    void setPreservePaths(bool value);
    void setChangeToFirstPath(bool value);
    void setImmediateProgressReporting(bool immediateProgressReporting);
    static QString findCommonPrefixForUrls(const QList<QUrl> &urls);
    static QString getFileNameForEntries(const QList<Archive::Entry *> &entries, const QString &suffix);
    static QString getFileNameForUrls(const QList<QUrl> &entries, const QString &suffix);

    QString fileName() const
    {
        return m_filename;
    }

public Q_SLOTS:
    bool addInput(const QUrl &url);
    void setAutoFilenameSuffix(const QString &suffix);
    void setFilename(const QUrl &path);
    void setMimeType(const QString &mimeType);
    void setPassword(const QString &password);
    void setHeaderEncryptionEnabled(bool enabled);
    void start() override;

protected:
    bool doKill() override;

private Q_SLOTS:
    void slotFinished(KJob *);
    void slotStartJob();

private:
    void detectFileName();
    bool confirmAndDelete(const QString &targetFile);

    CompressionOptions m_options;
    CreateJob *m_createJob = nullptr;
    QString m_filename;
    QString m_strippedPath;
    QString m_autoFilenameSuffix;
    QString m_firstPath;
    QString m_mimeType;
    QString m_password;
    QList<Archive::Entry *> m_entries;
    bool m_changeToFirstPath;
    bool m_enableHeaderEncryption;
    bool m_immediateProgressReporting;
};
}

#endif // ADDTOARCHIVE_H

/*
    SPDX-FileCopyrightText: 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef SINGLEFILEPLUGIN_H
#define SINGLEFILEPLUGIN_H

#include "archiveinterface.h"

class LibSingleFileInterface : public Kerfuffle::ReadOnlyArchiveInterface
{
    Q_OBJECT

public:
    LibSingleFileInterface(QObject *parent, const QVariantList & args);
    ~LibSingleFileInterface() override;

    bool list() override;
    bool testArchive() override;
    bool extractFiles(const QVector<Kerfuffle::Archive::Entry*> &files, const QString &destinationDirectory, const Kerfuffle::ExtractionOptions &options) override;

protected:
    const QString uncompressedFileName() const;
    QString overwriteFileName(QString& filename);

    QString m_mimeType;
    QStringList m_possibleExtensions;
};

#endif // SINGLEFILEPLUGIN_H

/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#ifndef KARCHIVEPLUGIN_H
#define KARCHIVEPLUGIN_H
#include "kerfuffle/archiveinterface.h"

#include <qplatformdefs.h> // mode_t

using namespace Kerfuffle;

class KArchive;
class KArchiveEntry;
class KArchiveDirectory;

class KERFUFFLE_EXPORT KArchiveInterface: public ReadWriteArchiveInterface
{
    Q_OBJECT
public:
    explicit KArchiveInterface(QObject *parent = 0, const QVariantList &args = QVariantList());
    ~KArchiveInterface();

    bool list() Q_DECL_OVERRIDE;
    bool copyFiles(const QList<QVariant> &files, const QString &destinationDirectory, ExtractionOptions options) Q_DECL_OVERRIDE;

    bool addFiles(const QStringList &files, const CompressionOptions &options) Q_DECL_OVERRIDE;
    bool deleteFiles(const QList<QVariant> & files) Q_DECL_OVERRIDE;

private:
    enum {
        OverwriteYes,
        OverwriteSkip,
        OverwriteAll,
        OverwriteAutoSkip,
        OverwriteCancel
    };

    bool browseArchive(KArchive *archive);

    bool processDir(const KArchiveDirectory *dir, const QString & prefix = QString());

    void createEntryFor(const KArchiveEntry *aentry, const QString& prefix);

    QString permissionsString(mode_t perm);

    void getAllEntries(const KArchiveDirectory *dir, const QString &prefix, QList< QVariant > &list);

    int handleFileExistsMessage(const QString &dir, const QString &fileName);

    KArchive *archive();

    KArchive *m_archive;
};

#endif // KARCHIVEPLUGIN_H

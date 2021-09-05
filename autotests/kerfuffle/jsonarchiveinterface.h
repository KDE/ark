/*
    SPDX-FileCopyrightText: 2010-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef JSONARCHIVEINTERFACE_H
#define JSONARCHIVEINTERFACE_H

#include "jsonparser.h"
#include "archiveinterface.h"

/**
 * A dummy archive interface used by our test cases.
 *
 * It reads a JSON file which defines the contents of the archive.
 * For the file format description, see the documentation for @c JSONParser.
 *
 * The file's content is read to memory when open() is called and the archive
 * is then closed. This means that this class never changes the file's content
 * on disk, and entry addition or deletion do not change the original file.
 *
 * @sa JSONParser
 *
 * @author Raphael Kubo da Costa <rakuco@FreeBSD.org>
 */
class JSONArchiveInterface : public Kerfuffle::ReadWriteArchiveInterface
{
    Q_OBJECT

public:
    explicit JSONArchiveInterface(QObject *parent, const QVariantList& args);
    ~JSONArchiveInterface() override;

    bool list() override;
    bool open() override;

    bool addFiles(const QVector<Kerfuffle::Archive::Entry*>& files, const Kerfuffle::Archive::Entry *destination, const Kerfuffle::CompressionOptions& options, uint numberOfEntriesToAdd = 0) override;
    bool moveFiles(const QVector<Kerfuffle::Archive::Entry*>& files, Kerfuffle::Archive::Entry *destination, const Kerfuffle::CompressionOptions& options) override;
    bool copyFiles(const QVector<Kerfuffle::Archive::Entry*>& files, Kerfuffle::Archive::Entry *destination, const Kerfuffle::CompressionOptions& options) override;
    bool extractFiles(const QVector<Kerfuffle::Archive::Entry*>& files, const QString &destinationDirectory, const Kerfuffle::ExtractionOptions& options) override;
    bool deleteFiles(const QVector<Kerfuffle::Archive::Entry*>& files) override;
    bool addComment(const QString& comment) override;
    bool testArchive() override;

private:
    JSONParser::JSONArchive m_archive;
};

#endif

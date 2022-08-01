/*
    SPDX-FileCopyrightText: 2022 Ilya Pominov <ipominov@astralinux.ru>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CLIPLUGIN_H
#define CLIPLUGIN_H

#include "cliinterface.h"

using namespace Kerfuffle;

struct ArjFileEntry;

class CliPlugin : public Kerfuffle::CliInterface
{
    Q_OBJECT

public:
    explicit CliPlugin(QObject *parent, const QVariantList & args);
    ~CliPlugin() override;

    bool addFiles(const QVector<Kerfuffle::Archive::Entry*> &files, const Kerfuffle::Archive::Entry *destination, const Kerfuffle::CompressionOptions& options, uint numberOfEntriesToAdd = 0) override;
    bool moveFiles(const QVector<Kerfuffle::Archive::Entry*> &files, Kerfuffle::Archive::Entry *destination, const Kerfuffle::CompressionOptions& options) override;

    void resetParsing() override;
    bool readListLine(const QString &line) override;
    bool readExtractLine(const QString &line) override;
    bool isFileExistsMsg(const QString &line) override;
    bool isFileExistsFileName(const QString &line) override;
    bool isNewMovedFileNamesMsg(const QString &line) override;

protected:
    bool handleLine(const QString& line) override;

protected Q_SLOTS:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus) override;

private:
    enum ParseState {
        ParseStateTitle,
        ParseStateProcessing,
        ParseStateArchiveDateTime,
        ParseStateArchiveComments,
        ParseStateEntryFileHeader,
        ParseStateEntryFileName,
        ParseStateEntryFileProperty,
        ParseStateEntryFileDTA,
        ParseStateEntryFileDTC,
        ParseStateEntryTotal,
    } m_parseState;

    void setupCliProperties();
    void ignoreLines(int lines, ParseState nextState);
    bool tryAddCurFileProperties(const QString &line);
    bool tryAddCurFileComment(const QString &line);
    void sendCurFileEntry();
    bool readLine(const QString& line);

    int m_remainingIgnoreLines = 0;
    QStringList m_headerComment;
    QScopedPointer <ArjFileEntry> m_currentParsedFile;
    bool m_testPassed = true;
    QVector<Archive::Entry*> m_renamedFiles;
};

#endif // CLIPLUGIN_H

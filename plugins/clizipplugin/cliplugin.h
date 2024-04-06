/*
    SPDX-FileCopyrightText: 2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CLIPLUGIN_H
#define CLIPLUGIN_H

#include "cliinterface.h"

using namespace Kerfuffle;

class CliPlugin : public Kerfuffle::CliInterface
{
    Q_OBJECT

public:
    explicit CliPlugin(QObject *parent, const QVariantList &args);
    ~CliPlugin() override;

    void resetParsing() override;
    QString escapeFileName(const QString &fileName) const override;
    bool readListLine(const QString &line) override;
    bool readExtractLine(const QString &line) override;
    bool isPasswordPrompt(const QString &line) override;
    bool isWrongPasswordMsg(const QString &line) override;
    bool isCorruptArchiveMsg(const QString &line) override;
    bool isDiskFullMsg(const QString &line) override;
    bool isFileExistsMsg(const QString &line) override;
    bool isFileExistsFileName(const QString &line) override;

    bool moveFiles(const QList<Archive::Entry *> &files, Archive::Entry *destination, const CompressionOptions &options) override;
    int moveRequiredSignals() const override;

private Q_SLOTS:
    void continueMoving(bool result);

private:
    void setupCliProperties();
    bool setMovingAddedFiles();
    void finishMoving(bool result);
    QString convertCompressionMethod(const QString &method);

    enum ParseState { ParseStateHeader = 0, ParseStateComment, ParseStateEntry } m_parseState;

    int m_linesComment;
    QString m_tempComment;
};

#endif // CLIPLUGIN_H

/*
    SPDX-FileCopyrightText: 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2009-2010 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2015-2016 Ragnar Thomsen <rthomsen6@gmail.com>
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CLIPLUGIN_H
#define CLIPLUGIN_H

#include "cliinterface.h"

class CliPlugin : public Kerfuffle::CliInterface
{
    Q_OBJECT

public:
    explicit CliPlugin(QObject *parent, const QVariantList &args);
    ~CliPlugin() override;

    void resetParsing() override;
    bool readListLine(const QString &line) override;
    bool readExtractLine(const QString &line) override;
    bool hasBatchExtractionProgress() const override;
    bool isPasswordPrompt(const QString &line) override;
    bool isWrongPasswordMsg(const QString &line) override;
    bool isCorruptArchiveMsg(const QString &line) override;
    bool isDiskFullMsg(const QString &line) override;
    bool isFileExistsMsg(const QString &line) override;
    bool isFileExistsFileName(const QString &line) override;
    bool isLocked() const override;

private:

    enum ParseState {
        ParseStateTitle = 0,
        ParseStateComment,
        ParseStateHeader,
        ParseStateEntryFileName,
        ParseStateEntryDetails,
        ParseStateLinkTarget
    } m_parseState;

    void setupCliProperties();

    bool handleUnrar5Line(const QString &line);
    void handleUnrar5Entry();
    bool handleUnrar4Line(const QString &line);
    void handleUnrar4Entry();
    void ignoreLines(int lines, ParseState nextState);

    QStringList m_unrar4Details;
    QHash<QString, QString> m_unrar5Details;

    QString m_unrarVersion;
    bool m_isUnrar5;
    bool m_isPasswordProtected;
    bool m_isSolid;
    bool m_isRAR5;
    bool m_isLocked;

    int m_remainingIgnoreLines;
    int m_linesComment;
};

#endif // CLIPLUGIN_H

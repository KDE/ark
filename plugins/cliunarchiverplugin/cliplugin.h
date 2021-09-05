/*
    SPDX-FileCopyrightText: 2011 Luke Shumaker <lukeshu@sbcglobal.net>
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

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

    bool list() override;
    bool extractFiles(const QVector<Kerfuffle::Archive::Entry*> &files, const QString &destinationDirectory, const Kerfuffle::ExtractionOptions &options) override;
    void resetParsing() override;
    bool readListLine(const QString &line) override;
    bool readExtractLine(const QString &line) override;
    bool isPasswordPrompt(const QString &line) override;

    /**
     * Fill the lsar's json output all in once (useful for unit testing).
     */
    void setJsonOutput(const QString &jsonOutput);

protected Q_SLOTS:
    void readStdout(bool handleAll = false) override;

protected:

    bool handleLine(const QString& line) override;

private Q_SLOTS:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus) override;

private:
    void setupCliProperties();
    void readJsonOutput();

    QString m_jsonOutput;
};

#endif // CLIPLUGIN_H

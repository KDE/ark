/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2011 Luke Shumaker <lukeshu@sbcglobal.net>
 * Copyright (C) 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>
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

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

#include "kerfuffle/archiveentry.h"
#include "kerfuffle/cliinterface.h"

class CliPlugin : public Kerfuffle::CliInterface
{
    Q_OBJECT

public:
    explicit CliPlugin(QObject *parent, const QVariantList &args);
    virtual ~CliPlugin();

    virtual bool list() Q_DECL_OVERRIDE;
    virtual bool extractFiles(const QVector<Kerfuffle::Archive::Entry*> &files, const QString &destinationDirectory, const Kerfuffle::ExtractionOptions &options) Q_DECL_OVERRIDE;
    virtual void resetParsing() Q_DECL_OVERRIDE;
    virtual bool readListLine(const QString &line) Q_DECL_OVERRIDE;

    /**
     * Fill the lsar's json output all in once (useful for unit testing).
     */
    void setJsonOutput(const QString &jsonOutput);

protected slots:
    void readStdout(bool handleAll = false) Q_DECL_OVERRIDE;

protected:

    bool handleLine(const QString& line) Q_DECL_OVERRIDE;

private slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus) Q_DECL_OVERRIDE;

private:
    void setupCliProperties();
    void readJsonOutput();

    QString m_jsonOutput;
};

#endif // CLIPLUGIN_H

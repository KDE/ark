/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2011 Raphael Kubo da Costa <kubito@gmail.com>
 * Copyright (C) 2012 basysKom GmbH <info@basyskom.com>
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
 */

#ifndef CLIPLUGIN_H
#define CLIPLUGIN_H

#include "kerfuffle/cliinterface.h"

using namespace Kerfuffle;

class KTemporaryFile;

class CliPlugin : public Kerfuffle::CliInterface
{
    Q_OBJECT

public:
    explicit CliPlugin(QObject *parent, const QVariantList &args);
    virtual ~CliPlugin();

    virtual bool copyFiles(const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options);
    virtual bool testFiles(const QList<QVariant> & files, TestOptions options = TestOptions());

    virtual QString escapeFileName(const QString &fileName) const;

    virtual Kerfuffle::ParameterList parameterList() const;

    virtual bool readListLine(const QString &line);
    virtual void resetReadState();

private slots:
    void readStdout2();

private:
    enum {
        Header = 0,
        Entry
    } m_status;

    enum {
        ZIP,
        ZIPX,
        WINZIP
    } m_filenameFormat;

    bool joinVolumes();
    bool runProcess2(const QString & programPath, const QStringList & args, const bool waitForFinished = true);
    void cleanUp();
    QString createTmpDir();

    QString m_tmpDir;
    KProcess * m_process;
    KTemporaryFile * m_tempFile;
    QString tmpDir;
};

#endif // CLIPLUGIN_H

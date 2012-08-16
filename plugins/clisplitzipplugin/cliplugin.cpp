/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2011 Raphael Kubo da Costa <kubito@gmail.com>
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

#include "cliplugin.h"
#include "kerfuffle/cliinterface.h"
#include "kerfuffle/kerfuffle_export.h"

#include <KDebug>
#include <KProcess>
#include <KStandardDirs>
#include <KTemporaryFile>

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QLatin1String>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <klocale.h>

using namespace Kerfuffle;

CliPlugin::CliPlugin(QObject *parent, const QVariantList & args)
    : CliInterface(parent, args)
    , m_status(Header)
    , m_filenameFormat(ZIP)
    , m_process(0)
    , m_tempFile(0)
{
    int i = m_filename.lastIndexOf(QLatin1String(".zip.z01"));
    if (i != -1) {
        // WinZip can create archives with ".zip.z01" extension.
        m_filenameFormat = WINZIP;
        m_filename.remove(i+4, 4);
        kDebug(1601) << "filename format is WINZIP";
    } else {
        i = m_filename.lastIndexOf(QLatin1String(".z01"));
        if (i != -1) {
            // zip command only processes files with this extension.
            m_filename.replace(i, 4, QLatin1String(".zip"));
            kDebug(1601) << "filename format is ZIP";
        } else {
            // WinZip can also create archives with ".zx01" extension.
            i = m_filename.lastIndexOf(QLatin1String(".zx01"));
    
            if (i != -1) {
                m_filenameFormat = ZIPX;
                m_filename.replace(i, 5, QLatin1String(".zipx"));
                kDebug(1601) << "filename format is ZIPX";
            } else {
                kDebug(1601) << "filename format not detected, assuming ZIP format";
            }
        }
    }
    kDebug(1601) << "Will open filename" << m_filename;
}

CliPlugin::~CliPlugin()
{
}

bool CliPlugin::copyFiles(const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options)
{
    if (!joinVolumes()) {
        return false;
    }

    QString originalFilename = m_filename;
    m_filename = m_tempFile->fileName();
    const bool ret = CliInterface::copyFiles(files, destinationDirectory, options);
    m_filename = originalFilename;

    cleanUp();
    return ret;
}

bool CliPlugin::testFiles(const QList<QVariant> & files, TestOptions options)
{
    if (!joinVolumes()) {
        return false;
    }

    QString originalFilename = m_filename;
    m_filename = m_tempFile->fileName();
    const bool ret = CliInterface::testFiles(files, options);
    m_filename = originalFilename;

    cleanUp();
    return ret;
}

bool CliPlugin::joinVolumes()
{
    // zip command requires volumes named as .z01, .z02, ..., .zip
    // WinZip names them as .zip.z01, .zip.z02, ..., .zip or .zx01, .zx02, ..., .zipx.
    // Here we create symbolic links so that zip command is happy. This also helps with
    // another problem: 'zip -s-' creates temporary files in archive's parent dir,
    // if the parent dir is not writable then 'zip -s-' will fail.
    createTmpDir();

    int v = m_filename.lastIndexOf(QDir::separator());
    QString sourceDir = m_filename.left(v+1); // directory where the original archive is located
    QString templateFilename = m_filename.mid(v+1);
    v = templateFilename.lastIndexOf(QLatin1Char('.'));
    templateFilename = templateFilename.left(v); // archive name without extension
    QString templateSource; // file name template for volumes in sourceDir
    QString templateLink = templateFilename + QLatin1String(".z"); // file name template for symbolic links in tmpDir

    QString volumeName; // volume file name in sourceDir. The extention will be added later
    QString linkName = templateFilename + QLatin1String(".zip"); // symbolic link name in tmpDir

    // first process the last volume, the one with .zip or .zipx extension.
    switch (m_filenameFormat) {
    case ZIP:
        templateSource = templateFilename + QLatin1String(".z");
        volumeName = templateFilename + QLatin1String(".zip");
        break;

    case ZIPX:
        templateSource = templateFilename + QLatin1String(".zx");
        volumeName = templateFilename + QLatin1String(".zipx");
        break;

    case WINZIP:
        templateSource = templateFilename + QLatin1String(".zip.z");
        volumeName = templateFilename + QLatin1String(".zip");
        break;
    }
    kDebug(1601) << "templateFilename" << templateFilename;

    v = 0;

    while (QFile::exists(sourceDir + volumeName)) {
        kDebug(1601) << "symlinking" << (sourceDir + volumeName) << "to" << (tmpDir + linkName);
        QFile::link(sourceDir + volumeName, tmpDir + linkName);
        ++v;
        volumeName = templateSource + QString(QLatin1String("%1")).arg(v, 2, 10, QLatin1Char('0'));
        linkName = templateLink + QString(QLatin1String("%1")).arg(v, 2, 10, QLatin1Char('0'));
    }

    // temporary file with all volumes joined.
    m_tempFile = new KTemporaryFile();
    m_tempFile->setSuffix(QLatin1String(".zip")); // required by zip command
    if (!m_tempFile->open()) {
        error(i18nc("@info", "Failed to create temporary file to join volumes."));
        finished(false);
        cleanUp();
        return false;
    }

    const QString programPath(KStandardDirs::findExe(QLatin1String("zip")));
    QStringList args;
    args.append(QLatin1String("-s-"));
    args.append(tmpDir + templateFilename + QLatin1String(".zip"));
    args.append(QLatin1String("-O"));
    args.append(m_tempFile->fileName());

    if (!runProcess(QStringList() << programPath, args)) {
        error(i18nc("@info", "Failed to join volumes."));
        finished(false);
        cleanUp();
        return false;
    }

    return true;
}

void CliPlugin::cleanUp()
{
    m_tempFile->deleteLater();
    m_tempFile = 0;

    QDir tempDir(tmpDir);
    QStringList list = tempDir.entryList(QDir::AllDirs | QDir::Files | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
    foreach (const QString & file, list) {
        kDebug(1601) << "removing" << (tmpDir + file);
        QFile::remove(tmpDir + file);
    }
    kDebug(1601) << "removing" << tmpDir;
    tempDir.rmpath(tmpDir);
}

QString CliPlugin::createTmpDir()
{
    QString temp = KGlobal::dirs()->findDirs("tmp", QLatin1String(""))[0];
    if (temp.isEmpty()) {
        temp = QLatin1String("/tmp/");
    }
    temp += QString(QLatin1String("CliSplitZipPlugin%1")).arg(QCoreApplication::applicationPid());

    int i = 0;
    tmpDir = temp;
    QDir dir(tmpDir);
    dir.rmpath(tmpDir);
    while (QDir(tmpDir).exists()) {
        tmpDir = temp + QString(QLatin1String("_%1")).arg(++i);
        dir.rmpath(tmpDir);
    }
    tmpDir += QLatin1Char('/');
    dir.mkpath(tmpDir);

    return tmpDir;
}

// #208091: infozip applies special meanings to some characters, so we
//          need to escape them with backslashes.see match.c in
//          infozip's source code
QString CliPlugin::escapeFileName(const QString &fileName) const
{
#if 0
    return fileName;
#else
    const QString escapedCharacters(QLatin1String("[]*?^-\\!"));

    QString quoted;
    const int len = fileName.length();
    const QLatin1Char backslash('\\');
    quoted.reserve(len * 2);

    for (int i = 0; i < len; ++i) {
        if (escapedCharacters.contains(fileName.at(i))) {
            quoted.append(backslash);
        }

        quoted.append(fileName.at(i));
    }

    return quoted;
#endif
}

ParameterList CliPlugin::parameterList() const
{
    static ParameterList p;

    if (p.isEmpty()) {
        p[CaptureProgress] = false;
        p[ListProgram] = QLatin1String( "zipinfo" );
        p[ExtractProgram] = QLatin1String( "unzip" );
        //p[DeleteProgram] = p[AddProgram] = QLatin1String( "zip" );

        // -U forces unzip to escape all non-ASCII characters from UTF-8 coded filenames as ''#Uxxxx''.
        // Actually, zipinfo uses UTF-16, not UTF-8, so we will need to parse the text to convert it to UTF-8.
        p[ListArgs] = QStringList() << QLatin1String( "-U" ) <<  QLatin1String( "-l" ) << QLatin1String( "-T" ) << QLatin1String( "$Archive" );
        p[ExtractArgs] = QStringList() << QLatin1String( "$PreservePathSwitch" ) << QLatin1String( "$PasswordSwitch" ) << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );
        p[PreservePathSwitch] = QStringList() << QLatin1String( "" ) << QLatin1String( "-j" );
        p[PasswordSwitch] = QStringList() << QLatin1String( "-P$Password" );

        //p[DeleteArgs] = QStringList() << QLatin1String( "-d" ) << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );

        p[FileExistsExpression] = QLatin1String( "^replace (.+)\\?" );
        p[FileExistsInput] = QStringList()
                             << QLatin1String( "y" ) //overwrite
                             << QLatin1String( "n" ) //skip
                             << QLatin1String( "A" ) //overwrite all
                             << QLatin1String( "N" ) //autoskip
                             ;

        //p[AddArgs] = QStringList() << QLatin1String( "$CompressionLevelSwitch" ) << QLatin1String( "-r" ) << QLatin1String( "$Archive" ) << QLatin1String( "$Files" );

        //p[CompressionLevelSwitches] = QStringList() << QLatin1String( "-0" ) << QLatin1String( "-3" ) << QLatin1String( "-6" ) << QLatin1String("-8" ) << QLatin1String("-9" );
        p[PasswordPromptPattern] = QLatin1String(" password: ");
        p[WrongPasswordPatterns] = QStringList() << QLatin1String( "incorrect password" ) << QLatin1String( "password incorrect" ) << QLatin1String( "Wrong password" ) ;
        p[TestProgram] = QLatin1String( "zip" );
        // Zip just supports a full archive check:
        p[TestArgs] = QStringList() << QLatin1String( "-T" ) << QLatin1String( "$PasswordSwitch" )  << QLatin1String( "$Archive" );
        p[TestFailedPatterns] = QStringList() << QLatin1String("FAILED") << QLatin1String("zip error");
        p[ExtractionFailedPatterns] = QStringList() << QLatin1String("CRC failed") << QLatin1String("Could not find");
    }
    return p;
}

bool CliPlugin::readListLine(const QString &l)
{
    static const QRegExp entryPattern(QLatin1String(
        "^(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\S+)\\s+(\\d{8}).(\\d{6})\\s+(.+)$") );
 
    // parse UTF-16 manually since zipinfo does not help us here :-(
    QString line = l;
    int pos = 0;
    ushort u = 0;
    bool ok;
    ushort unicode[2];
    while ((pos = line.indexOf(QLatin1String("#U"), pos)) != -1) {
        if (line.size() < (pos + 6)) {
            continue;
        }

        QString code = line[pos+2] + line[pos+3] + line[pos+4] + line[pos+5];
        u = code.toInt(&ok, 16);
        if (!ok) {
            continue;
        }

        // assuming little-endian.
        unicode[0] = u & 0x00ff;
        unicode[1] = u & 0xff00;
        code = QLatin1String("#U") + code;
        //kDebug(1601) << "replacing" << code << u << "with" << QString::fromUtf16(unicode);
        line.replace(code, QString::fromUtf16(unicode));
    }

    //kDebug(1601) << l;
    //kDebug(1601) << line;

    switch (m_status) {
    case Header:
        m_status = Entry;
        break;
    case Entry:
        if (entryPattern.indexIn(line) != -1) {
            ArchiveEntry e;
            e[Permissions] = entryPattern.cap(1);

            // #280354: infozip may not show the right attributes for a given directory, so an entry
            //          ending with '/' is actually more reliable than 'd' bein in the attributes.
            e[IsDirectory] = entryPattern.cap(10).endsWith(QLatin1Char('/'));

            e[Size] = entryPattern.cap(4).toInt();
            QString status = entryPattern.cap(5);
            if (status[0].isUpper()) {
                e[IsPasswordProtected] = true;
            }
            e[CompressedSize] = entryPattern.cap(6).toInt();

            const QDateTime ts(QDate::fromString(entryPattern.cap(8), QLatin1String( "yyyyMMdd" )),
                               QTime::fromString(entryPattern.cap(9), QLatin1String( "hhmmss" )));
            e[Timestamp] = ts;

            QString resultString = entryPattern.cap(10);
            e[InternalID] = resultString;
            e[FileName] = resultString;
            entry(e);
        }
        break;
    }

    return true;
}

void CliPlugin::resetReadState()
{
    m_status = Header;
}

KERFUFFLE_EXPORT_PLUGIN(CliPlugin)


/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv atatatat stud.ntnu.no>
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
#ifndef _CLIINTERFACE_H_
#define _CLIINTERFACE_H_

#include "archiveinterface.h"
#include "kerfuffle_export.h"
#include <QtCore/QProcess>
#include <QtCore/QRegExp>

class KProcess;

namespace Kerfuffle
{
enum CliInterfaceExtractOptions {
};

enum CliInterfaceParameters {

    ///////////////[ COMMON ]/////////////

    /**
     * Bool (default false)
     * Will look for the %-sign in the stdout while working, in the form of
     * (2%, 14%, 35%, etc etc), and report progress based upon this
     */
    CaptureProgress = 0,

    ///////////////[ LIST ]/////////////

    /**
     * QString
     * The name to the program that will handle listing of this
     * archive (eg "rar"). Will be searched for in PATH
     */
    ListProgram,
    /**
     * QStringList
     * The arguments that are passed to the program above for
     * listing the archive. Special strings that will be
     * substituted:
     * $Archive - the path of the archive
     */
    ListArgs,

    ///////////////[ EXTRACT ]/////////////

    /**
     * QString
     * The name to the program that will handle extracting of this
     * archive (eg "rar"). Will be searched for in PATH
     */
    ExtractProgram,
    /**
     * QStringList
     * The arguments that are passed to the program above for
     * extracting the archive. Special strings that will be
     * substituted:
     * $Archive - the path of the archive
     * $Files - the files selected to be extracted, if any
     * $PreservePathSwitch - the flag for extracting with full paths
     * $RootNodeSwitch - the internal work dir in the archive (for example
     * when the user has dragged a folder from the archive and wants it
     * extracted relative to it)
     * $PasswordSwitch - the switch setting the password. Note that this
     * will not be inserted unless the listing function has emitted an
     * entry with the IsPasswordProtected property set to true.
     */
    ExtractArgs,
    /**
     * Bool (default false)
     * When passing directories to the extract program, do not
     * include trailing slashes
     * e.g. if the user selected "foo/" and "foo/bar" in the gui, the
     * paths "foo" and "foo/bar" will be sent to the program.
     */
    NoTrailingSlashes,
    /**
     * QStringList
     * This should be a qstringlist with either two elements. The first
     * string is what PreservePathSwitch in the ExtractArgs will be replaced
     * with if PreservePath is True/enabled. The second is for the disabled
     * case. An empty string means that the argument will not be used in
     * that case.
     * Example: for rar, "x" means extract with full paths, and "e" means
     * extract without full paths. in this case we will use the stringlist
     * ("x", "e"). Or, for another format that might use the switch
     * "--extractFull" for preservePaths, and nothing otherwise: we use the
     * stringlist ("--preservePaths", "")
     */
    PreservePathSwitch,
    /**
     * QStringList (default empty)
     * The format of the root node switch. The variable $Path will be
     * substituted for the path string.
     * Example: ("--internalPath=$Path)
     * or ("--path", "$Path")
     */
    RootNodeSwitch,
    /**
     * QStringList (default empty)
     * The format of the root node switch. The variable $Password will be
     * substituted for the password string. NOTE: supplying passwords
     * through a virtual terminal is not supported (yet?), because this
     * is not cross platform compatible. As of KDE 4.3 there are no plans to
     * change this.
     * Example: ("-p$Password)
     * or ("--password", "$Password")
     */
    PasswordSwitch,
    /**
     * QString
     * This is a regexp, defining how to recognize a "File already exists"
     * prompt when extracting. It should have one captured string, which is
     * the filename of the file/folder that already exists.
     */
    FileExistsExpression,
    /**
     * int
     * This sets on what output channel the FileExistsExpression regex
     * should be applied on, in other words, on what stream the "file
     * exists" output will appear in. Values accepted:
     * 0 - Standard error, stderr (default)
     * 1 - Standard output, stdout
     */
    FileExistsMode,
    /**
     * QStringList
     * The various responses that can be supplied as a response to the
     * "file exists" prompt. The various items are to be supplied in the
     * following order:
     * index 0 - Yes (overwrite)
     * index 1 - No (skip/do not overwrite)
     * index 2 - All (overwrite all)
     * index 3 - Cancel operation
     * index 4 - Do not overwrite any files (autoskip)
     */
    FileExistsInput,

    ///////////////[ DELETE ]/////////////

    /**
     * QString
     * The name to the program that will handle deleting of elements in this
     * archive format (eg "rar"). Will be searched for in PATH
     */
    DeleteProgram,
    /**
     * QStringList
     * The arguments that are passed to the program above for
     * deleting from the archive. Special strings that will be
     * substituted:
     * $Archive - the path of the archive
     * $Files - the files selected to be deleted
     */
    DeleteArgs,
    /**
     * QStringList
     * Default: empty
     * A list of regexp patterns that will cause the extraction to exit
     * with a general fail message
     */
    ExtractionFailedPatterns,
    /**
     * QStringList
     * Default: empty
     * A list of regexp patterns that will alert the user that the password
     * was wrong.
     */
    WrongPasswordPatterns,

    ///////////////[ ADD ]/////////////

    /**
     * QString
     * The name to the program that will handle adding in this
     * archive format (eg "rar"). Will be searched for in PATH
     */
    AddProgram,
    /**
     * QStringList
     * The arguments that are passed to the program above for
     * adding to the archive. Special strings that will be
     * substituted:
     * $Archive - the path of the archive
     * $Files - the files selected to be added
     */
    AddArgs
};

typedef QHash<int, QVariant> ParameterList;

class KERFUFFLE_EXPORT CliInterface : public ReadWriteArchiveInterface
{
    Q_OBJECT

public:

    enum OperationMode  {
        List, Copy, Add, Delete
    };
    OperationMode m_mode;

    explicit CliInterface(const QString& filename, QObject *parent = 0);
    virtual ~CliInterface();

    virtual bool list();
    virtual bool copyFiles(const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options);
    virtual bool addFiles(const QStringList & files, const CompressionOptions& options);
    virtual bool deleteFiles(const QList<QVariant> & files);

    virtual ParameterList parameterList() const = 0;
    virtual bool readListLine(const QString &line) = 0;

private:
    bool findProgramAndCreateProcess(const QString& program);
    void substituteCopyVariables(QStringList& params, const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options);
    void substituteListVariables(QStringList& params);

    bool createProcess();
    bool executeProcess(const QString& path, const QStringList & args);
    void cacheParameterList();
    bool checkForFileExistsMessage(const QString& line);
    bool handleFileExistsMessage(const QString& filename);
    bool checkForErrorMessage(const QString& line, int parameterIndex);
    void handleLine(const QString& line);

    void failOperation();

    bool doKill();
    bool doSuspend();
    bool doResume();

    QByteArray m_stdOutData;
    bool m_userCancelled;
    QRegExp m_existsPattern;

    KProcess *m_process;
    QString m_program;
    ParameterList m_param;
    QVariantList m_removedFiles;

private slots:
    void started();
    void readStdout(bool handleAll = false);
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
};
}

#endif /* _CLIINTERFACE_H_ */

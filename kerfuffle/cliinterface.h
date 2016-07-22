/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef CLIINTERFACE_H
#define CLIINTERFACE_H

#include "archiveinterface.h"
#include "archiveentry.h"
#include "kerfuffle_export.h"

#include <QProcess>
#include <QRegularExpression>

class KProcess;
class KPtyProcess;

class QDir;
class QTemporaryDir;
class QTemporaryFile;

namespace Kerfuffle
{

enum CliInterfaceParameters {

    ///////////////[ COMMON ]/////////////

    /**
     * Bool (default false)
     * Will look for the %-sign in the stdout while working, in the form of
     * (2%, 14%, 35%, etc etc), and report progress based upon this
     */
    CaptureProgress = 0,

    /**
     * QString
     * Default: empty
     * A regexp pattern that matches the program's password prompt.
     */
    PasswordPromptPattern,

    ///////////////[ LIST ]/////////////

    /**
     * QStringList
     * The names to the program that will handle listing of this
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
    /**
     * QStringList (default empty)
     * List of regexp patterns that indicate a corrupt archive.
     */
    CorruptArchivePatterns,

    ///////////////[ EXTRACT ]/////////////

    /**
     * QStringList
     * The names to the program that will handle extracting of this
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
     * stringlist ("--extractFull", "")
     */
    PreservePathSwitch,
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
     * The format of the compression level switch. The variable $CompressionLevel
     * will be substituted for the level.
     * Example: ("-mx=$CompressionLevel)
     */
    CompressionLevelSwitch,
    /**
     * QStringList
     * This is a stringlist with regexps, defining how to recognize the last
     * line in a "File already exists" prompt when extracting.
     */
    FileExistsExpression,
    /**
     * QStringList
     * This is a stringlist with regexps defining how to recognize the line
     * containing the filename in a "File already exists" prompt when
     * extracting. It should have one captured string, which is the filename
     * of the file/folder that already exists.
     */
    FileExistsFileName,
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
     * index 3 - Do not overwrite any files (autoskip)
     * index 4 - Cancel operation
     */
    FileExistsInput,
    /**
      * QStringList
      * Regexp patterns capturing disk is full error messages.
      */
    DiskFullPatterns,

    ///////////////[ DELETE ]/////////////

    /**
     * QStringList
     * The names to the program that will handle deleting of elements in this
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
     * QStringList
     * The names to the program that will handle adding in this
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
    AddArgs,

    ///////////////[ ENCRYPT ]/////////////

    /**
     * QStringList (default empty)
     * The variable $Password will be
     * substituted for the password string used to encrypt the header.
     * Example (rar plugin): ("-hp$Password")
     */
    PasswordHeaderSwitch,

    ///////////////[ COMMENT ]/////////////

    /**
     * QStringList
     * The arguments that are passed to AddProgram when adding
     * a comment.
     */
    CommentArgs,
    /**
     * QString
     * The variable $CommentFile will be substituted for the file
     * containing the comment.
     * Example (rar plugin): -z$CommentFile
     */
    CommentSwitch,
    TestProgram,
    TestArgs,
    TestPassedPattern
};

typedef QHash<int, QVariant> ParameterList;

class KERFUFFLE_EXPORT CliInterface : public ReadWriteArchiveInterface
{
    Q_OBJECT

public:
    enum OperationMode  {
        List, Copy, Add, Delete, Comment, Test
    };
    OperationMode m_operationMode;

    explicit CliInterface(QObject *parent, const QVariantList & args);
    virtual ~CliInterface();

    virtual bool list() Q_DECL_OVERRIDE;
    virtual bool copyFiles(const QList<Archive::Entry*> &files, const QString& destinationDirectory, const ExtractionOptions& options) Q_DECL_OVERRIDE;
    virtual bool addFiles(const QList<Archive::Entry*> &files, const Archive::Entry *destination, const QString &tempDirPath, const CompressionOptions& options) Q_DECL_OVERRIDE;
    virtual bool deleteFiles(const QList<Archive::Entry*> &files) Q_DECL_OVERRIDE;
    virtual bool addComment(const QString &comment) Q_DECL_OVERRIDE;
    virtual bool testArchive() Q_DECL_OVERRIDE;

    virtual void resetParsing() = 0;
    virtual ParameterList parameterList() const = 0;
    virtual bool readListLine(const QString &line) = 0;
    bool doKill() Q_DECL_OVERRIDE;
    bool doSuspend() Q_DECL_OVERRIDE;
    bool doResume() Q_DECL_OVERRIDE;

    /**
     * Sets if the listing should include empty lines.
     *
     * The default value is false.
     */
    void setListEmptyLines(bool emptyLines);

    /**
     * Move all files from @p tmpDir to @p destDir, preserving paths if @p preservePaths is true.
     * @return Whether the operation has been successful.
     */
    bool moveToDestination(const QDir &tempDir, const QDir &destDir, bool preservePaths);

    QStringList substituteListVariables(const QStringList &listArgs, const QString &password);
    QStringList substituteCopyVariables(const QStringList &extractArgs, const QList<Archive::Entry*> &entries, bool preservePaths, const QString &password);
    QStringList substituteAddVariables(const QStringList &addArgs, const QList<Archive::Entry*> &entries, const QString &password, bool encryptHeader, int compLevel);
    QStringList substituteDeleteVariables(const QStringList &deleteArgs, const QList<Archive::Entry*> &entries, const QString &password);
    QStringList substituteCommentVariables(const QStringList &commentArgs, const QString &commentFile);
    QStringList substituteTestVariables(const QStringList &testArgs);

    /**
     * @return The preserve path switch, according to the @p preservePaths extraction option.
     */
    QString preservePathSwitch(bool preservePaths) const;

    /**
     * @return The password header-switch with the given @p password.
     */
    virtual QStringList passwordHeaderSwitch(const QString& password) const;

    /**
     * @return The password switch with the given @p password.
     */
    QStringList passwordSwitch(const QString& password) const;

    /**
     * @return The compression level switch with the given @p level.
     */
    QString compressionLevelSwitch(int level) const;

    /**
     * @return The list of selected files to extract.
     */
    QStringList copyFilesList(const QList<Archive::Entry*> &files) const;

protected:

    virtual void handleLine(const QString& line);
    virtual void cacheParameterList();

    /**
     * Run @p programName with the given @p arguments.
     *
     * @param programName The program that will be run (not the whole path).
     * @param arguments A list of arguments that will be passed to the program.
     *
     * @return @c true if the program was found and the process was started correctly,
     *         @c false otherwise (in which case finished(false) is emitted).
     */
    bool runProcess(const QStringList& programNames, const QStringList& arguments);

    /**
     * Kill the running process. The finished signal is emitted according to @p emitFinished.
     */
    void killProcess(bool emitFinished = true);

    /**
     * Ask the password *before* running any process.
     * @return True if the user supplies a password, false otherwise (in which case finished() is emitted).
     */
    bool passwordQuery();

    ParameterList m_param;
    int m_exitCode;

protected slots:
    virtual void readStdout(bool handleAll = false);

private:

    /**
     * Checks whether a line of the program's output is a password prompt.
     *
     * It uses the regular expression in the @c PasswordPromptPattern parameter
     * for the check.
     *
     * @param line A line of the program's output.
     *
     * @return @c true if the given @p line is a password prompt, @c false
     * otherwise.
     */

    bool checkForPasswordPromptMessage(const QString& line);

    bool handleFileExistsMessage(const QString& filename);
    bool checkForErrorMessage(const QString& line, int parameterIndex);
    bool checkForTestSuccessMessage(const QString& line);

    /**
     * Performs any additional escaping and processing on @p fileName
     * before passing it to the underlying process.
     *
     * The default implementation returns @p fileName unchanged.
     *
     * @param fileName String to escape.
     */
    virtual QString escapeFileName(const QString &fileName) const;

    /**
     * Wrapper around KProcess::write() or KPtyDevice::write(), depending on
     * the platform.
     */
    void writeToProcess(const QByteArray& data);

    bool moveDroppedFilesToDest(const QList<Archive::Entry*> &files, const QString &finalDest);

    /**
     * @return Whether @p dir is an empty directory.
     */
    bool isEmptyDir(const QDir &dir);

    void copyProcessCleanup();

    QByteArray m_stdOutData;
    QRegularExpression m_passwordPromptPattern;
    QHash<int, QList<QRegularExpression> > m_patternCache;

#ifdef Q_OS_WIN
    KProcess *m_process;
#else
    KPtyProcess *m_process;
#endif

    QList<Archive::Entry*> m_removedFiles;
    bool m_listEmptyLines;
    bool m_abortingOperation;
    QString m_storedFileName;

    CompressionOptions m_compressionOptions;
    QString m_oldWorkingDir;
    QString m_extractDestDir;
    QTemporaryDir *m_extractTempDir;
    QTemporaryFile *m_commentTempFile;
    QList<Archive::Entry*> m_copiedFiles;

private slots:
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void copyProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

};
}

#endif /* CLIINTERFACE_H */

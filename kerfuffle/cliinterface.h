/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (C) 2012 basysKom GmbH <info@basyskom.com>
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
#include "kerfuffle_export.h"
#include <QtCore/QProcess>
#include <QtCore/QRegExp>
#include <QtCore/QDateTime>

class KProcess;
class KPtyProcess;
class KUrl;

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

    /**
     * QString
     * Default: empty
     * A regexp pattern that matches the program's question whether to resuse the passsword for
     * the next file.
     */
    UseCurrentPasswordPattern,
    /**
     * QStringList
     * The various responses that can be supplied as a response to the
     * "Use current password" prompt. The various items are to be supplied in the
     * following order:
     * index 0 - Yes
     * index 1 - No
     * index 2 - All
     */
    UseCurrentPasswordInput,

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
     * stringlist ("--extractFull", "")
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
     * index 3 - Do not overwrite any files (autoskip)
     * index 4 - Cancel operation
     * The following items are optional and should be used with caution
     * index 5 - Rename (if supported, must be implemented as auto rename)
     */
    FileExistsInput,
    /**
     * QStringList
     * The commandline switch that can be supplied for handling of existing files:
     * index 0 - Ask before overwrite
     * index 1 - Overwrite all
     * index 2 - Skip all (do not overwrite)
     * index 3 - Auto rename
     */
    FileExistsSwitch,
    /**
     * Bool (default false),
     * CAUTION: Parameter shall be used only for feedback to the GUI.
     *
     */
    SupportsRename,
    /**
     * QString
     * This is a regexp, defining how to recognize a "Enter new name"
     * prompt when extracting a file that already exists and should be renamed.
     * This will only be useful if renaming is supported and not done by auto renaming (as with 7z).
     */
    FileExistsNewNamePattern,

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
    /**
     * QStringList
     * Patterns that indicate a fail during adding
     */
    AddFailedPatterns,
    /**
     * QStringList
     * The arguments that are passed to the program above for
     * setting the compression level.
     * First entry is "Store"
     * Second entry is "Fast"
     * Third entry is "Normal"
     * Fourth entry is "Good"
     * Fifth entry is "Maximum"
     */
    CompressionLevelSwitches,
    /**
     * QString
     * This is a string that allows to enable Multithreading for
     * compression and decompression. This only has some effect for some
     * cases.
     */
    MultiThreadingSwitch,
    /**
     * QStringList
     * The arguments that are passed to the program above for
     * setting the compression level.
     * First entry is "AES256"
     * Second entry is "ZipCrypto"
     */
    EncryptionMethodSwitches,
    /**
     * QString
     * This is a string that allows to enable encrypting headers for
     * compression. This only has some effect when supported.
     */
    EncryptHeaderSwitch,
    /**
     * QString
     * This is a string that allows to enable multipart support for
     * compression. This only has some effect when supported.
     * The variable $MultiPartSize will be replaced accordingly.
     * The file size is specified in kiloBytes.
     * Example: ("-v$MultiPartSizek")
     */
    MultiPartSwitch,
    /**
     * QString
     * The name to the program that will handle testing in this
     * archive format (eg "rar"). Will be searched for in PATH
     */
    TestProgram,
    /**
     * QStringList
     * The arguments that are passed to the program above for
     * testing the archive. Special strings that will be
     * substituted:
     * $Archive - the path of the archive
     * $Files - the files selected to be added
     */
    TestArgs,
    /**
     * QStringList
     * Patterns that indicate a fail during testing
     */
    TestFailedPatterns,
    /**
     * QString
     * Directory where temporary files will be created
     */
    TemporaryDirectorySwitch
};

typedef QHash<int, QVariant> ParameterList;

class KERFUFFLE_EXPORT CliInterface : public ReadWriteArchiveInterface
{
    Q_OBJECT

public:
    enum OperationMode  {
        List, Copy, Add, Delete, Test
    };
    OperationMode m_operationMode;

    explicit CliInterface(QObject *parent, const QVariantList & args);
    virtual ~CliInterface();

    virtual bool list();
    virtual bool copyFiles(const QList<QVariant> & files, const QString & destinationDirectory, ExtractionOptions options);
    virtual bool addFiles(const QStringList & files, const CompressionOptions& options);
    virtual bool deleteFiles(const QList<QVariant> & files);
    virtual bool testFiles(const QList<QVariant> & files, TestOptions options = TestOptions());

    virtual ParameterList parameterList() const = 0;
    virtual bool readListLine(const QString &line) = 0;
    virtual void resetReadState() = 0;

    // in case parsing code needs it, like when parsing 7z's extraction output to detect the filename that has caused conflict.
    virtual void saveLastLine(const QString &line) {
        Q_UNUSED(line);
    };
    virtual QString fileExistsName() {
        return QString();
    };

    bool doKill();
    bool doSuspend();
    bool doResume();

    /**
     * Returns the list of characters which are preceded by a
     * backslash when a file name in an archive is passed to
     * a program.
     *
     * @see setEscapedCharacters().
     */
    QString escapedCharacters();

    /**
     * Sets which characters will be preceded by a backslash when
     * a file name in an archive is passed to a program.
     *
     * @see escapedCharacters().
     */
    void setEscapedCharacters(const QString& characters);

    /**
     * Checks whether the implementing plugin supports the provided option
     */
    virtual bool supportsOption(const Kerfuffle::SupportedOptions option, const QString & mimeType = QString());

    /**
     * Does encoding detection for a filename using
     * KEncodingProber and filename specific criteria and
     * returns the encoding-corrected string
     */
    static QString autoConvertEncoding(const QString & fileName);

#ifndef Q_OS_WIN
signals:
    void quitEventLoop();
#endif

protected:
    static void fixFileNameEncoding(const QString & destinationDirectory, const QDateTime & timestamp = QDateTime());

    /**
     * Run @p programName with the given @p arguments.
     * The method waits until @p programName is finished to exit.
     *
     * @param programName The program that will be run (not the whole path).
     * @param arguments A list of arguments that will be passed to the program.
     *
     * @return @c true if the program was found and the process ran correctly,
     *         @c false otherwise.
     */
    bool runProcess(const QStringList& programNames, const QStringList& arguments);

    /**
     * Checks whether the implementing plugin supports the provided parameter
     */
    bool supportsParameter( CliInterfaceParameters param );

private Q_SLOTS:
    void killAllProcesses();

private:
    void substituteListVariables(QStringList& params);

    void cacheParameterList();

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

    bool checkForFileExistsMessage(const QString& line);
    bool handleFileExistsMessage(const QString& filename);
    bool checkForErrorMessage(const QString& line, int parameterIndex);
    void handleLine(const QString& line);
    bool checkForRenameFileMessage(const QString& line);
    bool handleRenameFileMessage(const QString& line);
    bool checkForUseCurrentPasswordMessage(const QString &line);

    void failOperation();

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

    QString findFileExistsName();
    static bool containsUmlaut(const QString s);

    QByteArray m_stdOutData;
    QRegExp m_existsPattern;
    QRegExp m_passwordPromptPattern;
    QRegExp m_renameFilePattern;
    QRegExp m_useCurrentPasswordPattern;

#ifdef Q_OS_WIN
    KProcess *m_process;
#else
    KPtyProcess *m_process;
#endif

    ParameterList m_param;
    QVariantList m_removedFiles;
    bool m_testResult;
    bool m_fixFileNameEncoding;
    bool m_alreadyFailed;
    QString m_destinationDirectory;
    QDateTime m_timestamp;
    QHash<QString, QVariant> m_options;

private slots:
    void readStdout(bool handleAll = false);
    void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
};
}

#endif /* CLIINTERFACE_H */

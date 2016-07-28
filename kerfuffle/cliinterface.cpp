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

#include "cliinterface.h"
#include "ark_debug.h"
#include "queries.h"

#ifdef Q_OS_WIN
# include <KProcess>
#else
# include <KPtyDevice>
# include <KPtyProcess>
#endif

#include <KLocalizedString>

#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QFile>
#include <QMimeDatabase>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QThread>
#include <QTimer>
#include <QUrl>

namespace Kerfuffle
{
CliInterface::CliInterface(QObject *parent, const QVariantList & args)
        : ReadWriteArchiveInterface(parent, args),
        m_process(0),
        m_abortingOperation(false),
        m_listEmptyLines(false),
        m_extractTempDir(Q_NULLPTR),
        m_commentTempFile(Q_NULLPTR)
{
    //because this interface uses the event loop
    setWaitForFinishedSignal(true);

    if (QMetaType::type("QProcess::ExitStatus") == 0) {
        qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    }
}

void CliInterface::cacheParameterList()
{
    m_param = parameterList();
    Q_ASSERT(m_param.contains(ExtractProgram));
    Q_ASSERT(m_param.contains(ListProgram));
    Q_ASSERT(m_param.contains(PreservePathSwitch));
    Q_ASSERT(m_param.contains(FileExistsExpression));
    Q_ASSERT(m_param.contains(FileExistsInput));
}

CliInterface::~CliInterface()
{
    Q_ASSERT(!m_process);
    delete m_commentTempFile;
}

void CliInterface::setListEmptyLines(bool emptyLines)
{
    m_listEmptyLines = emptyLines;
}

bool CliInterface::list()
{
    resetParsing();
    cacheParameterList();
    m_operationMode = List;

    const auto args = substituteListVariables(m_param.value(ListArgs).toStringList(), password());

    if (!runProcess(m_param.value(ListProgram).toStringList(), args)) {
        return false;
    }

    return true;
}

bool CliInterface::copyFiles(const QVariantList &files, const QString &destinationDirectory, const ExtractionOptions &options)
{
    qCDebug(ARK) << Q_FUNC_INFO << "to" << destinationDirectory;

    cacheParameterList();
    m_operationMode = Copy;
    m_compressionOptions = options;
    m_copiedFiles = files;
    m_extractDestDir = destinationDirectory;
    const QStringList extractArgs = m_param.value(ExtractArgs).toStringList();

    if (extractArgs.contains(QStringLiteral("$PasswordSwitch")) &&
        options.value(QStringLiteral("PasswordProtectedHint")).toBool() &&
        password().isEmpty()) {
        qCDebug(ARK) << "Password hint enabled, querying user";
        if (!passwordQuery()) {
            return false;
        }
    }

    // Populate the argument list.
    const QStringList args = substituteCopyVariables(extractArgs,
                                                     files,
                                                     options.value(QStringLiteral("PreservePaths")).toBool(),
                                                     password());

    QUrl destDir = QUrl(destinationDirectory);
    QDir::setCurrent(destDir.adjusted(QUrl::RemoveScheme).url());

    bool useTmpExtractDir = options.value(QStringLiteral("DragAndDrop")).toBool() ||
                            options.value(QStringLiteral("AlwaysUseTmpDir")).toBool();

    if (useTmpExtractDir) {

        Q_ASSERT(!m_extractTempDir);
        m_extractTempDir = new QTemporaryDir(QApplication::applicationName() + QLatin1Char('-'));

        qCDebug(ARK) << "Using temporary extraction dir:" << m_extractTempDir->path();
        if (!m_extractTempDir->isValid()) {
            qCDebug(ARK) << "Creation of temporary directory failed.";
            emit finished(false);
            return false;
        }
        m_oldWorkingDir = QDir::currentPath();
        destDir = QUrl(m_extractTempDir->path());
        QDir::setCurrent(destDir.adjusted(QUrl::RemoveScheme).url());
    }

    if (!runProcess(m_param.value(ExtractProgram).toStringList(), args)) {
        return false;
    }

    return true;
}

bool CliInterface::addFiles(const QStringList & files, const CompressionOptions& options)
{
    cacheParameterList();

    m_operationMode = Add;

    const QStringList addArgs = m_param.value(AddArgs).toStringList();

    if (addArgs.contains(QStringLiteral("$PasswordSwitch")) &&
        options.value(QStringLiteral("PasswordProtectedHint")).toBool() &&
        password().isEmpty()) {
        qCDebug(ARK) << "Password hint enabled, querying user";
        if (!passwordQuery()) {
            return false;
        }
    }

    int compLevel = options.value(QStringLiteral("CompressionLevel"), -1).toInt();
    ulong volumeSize = options.value(QStringLiteral("VolumeSize"), 0).toULongLong();

    const auto args = substituteAddVariables(m_param.value(AddArgs).toStringList(),
                                             files,
                                             password(),
                                             isHeaderEncryptionEnabled(),
                                             compLevel,
                                             volumeSize);

    if (!runProcess(m_param.value(AddProgram).toStringList(), args)) {
        return false;
    }

    return true;
}

bool CliInterface::deleteFiles(const QList<QVariant> & files)
{
    cacheParameterList();
    m_operationMode = Delete;

    m_removedFiles = files;

    const auto deleteArgs = m_param.value(DeleteArgs).toStringList();

    const auto args = substituteDeleteVariables(deleteArgs,
                                                files,
                                                password());

    if (!runProcess(m_param.value(DeleteProgram).toStringList(), args)) {
        return false;
    }

    return true;
}

bool CliInterface::testArchive()
{
    resetParsing();
    cacheParameterList();
    m_operationMode = Test;

    const auto args = substituteTestVariables(m_param.value(TestArgs).toStringList(), password());

    if (!runProcess(m_param.value(TestProgram).toStringList(), args)) {
        return false;
    }

    return true;
}

bool CliInterface::runProcess(const QStringList& programNames, const QStringList& arguments)
{
    Q_ASSERT(!m_process);

    QString programPath;
    for (int i = 0; i < programNames.count(); i++) {
        programPath = QStandardPaths::findExecutable(programNames.at(i));
        if (!programPath.isEmpty())
            break;
    }
    if (programPath.isEmpty()) {
        const QString names = programNames.join(QStringLiteral(", "));
        emit error(xi18ncp("@info", "Failed to locate program <filename>%2</filename> on disk.",
                           "Failed to locate programs <filename>%2</filename> on disk.", programNames.count(), names));
        emit finished(false);
        return false;
    }

    qCDebug(ARK) << "Executing" << programPath << arguments << "within directory" << QDir::currentPath();

#ifdef Q_OS_WIN
    m_process = new KProcess;
#else
    m_process = new KPtyProcess;
    m_process->setPtyChannels(KPtyProcess::StdinChannel);
#endif

    m_process->setOutputChannelMode(KProcess::MergedChannels);
    m_process->setNextOpenMode(QIODevice::ReadWrite | QIODevice::Unbuffered | QIODevice::Text);
    m_process->setProgram(programPath, arguments);

    connect(m_process, SIGNAL(readyReadStandardOutput()), SLOT(readStdout()), Qt::DirectConnection);

    if (m_operationMode == Copy) {
        // Extraction jobs need a dedicated post-processing function.
        connect(m_process, static_cast<void (KPtyProcess::*)(int, QProcess::ExitStatus)>(&KPtyProcess::finished), this, &CliInterface::copyProcessFinished, Qt::DirectConnection);
    } else {
        connect(m_process, static_cast<void (KPtyProcess::*)(int, QProcess::ExitStatus)>(&KPtyProcess::finished), this, &CliInterface::processFinished, Qt::DirectConnection);
    }

    m_stdOutData.clear();

    m_process->start();

    return true;
}

void CliInterface::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    m_exitCode = exitCode;
    qCDebug(ARK) << "Process finished, exitcode:" << exitCode << "exitstatus:" << exitStatus;

    if (m_process) {
        //handle all the remaining data in the process
        readStdout(true);

        delete m_process;
        m_process = Q_NULLPTR;
    }

    // #193908 - #222392
    // Don't emit finished() if the job was killed quietly.
    if (m_abortingOperation) {
        return;
    }

    if (m_operationMode == Delete) {
        foreach(const QVariant& v, m_removedFiles) {
            emit entryRemoved(v.toString());
        }
    }

    if (m_operationMode == Add && !isMultiVolume()) {
        list();
    } else if (m_operationMode == List && isCorrupt()) {
        Kerfuffle::LoadCorruptQuery query(filename());
        emit userQuery(&query);
        query.waitForResponse();
        if (!query.responseYes()) {
            emit cancelled();
            emit finished(false);
        } else {
            emit progress(1.0);
            emit finished(true);
        }
    } else  {
        emit progress(1.0);
        emit finished(true);
    }
}

void CliInterface::copyProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_ASSERT(m_operationMode == Copy);

    m_exitCode = exitCode;
    qCDebug(ARK) << "Extraction process finished, exitcode:" << exitCode << "exitstatus:" << exitStatus;

    if (m_process) {
        // Handle all the remaining data in the process.
        readStdout(true);

        delete m_process;
        m_process = Q_NULLPTR;
    }

    if (m_compressionOptions.value(QStringLiteral("AlwaysUseTmpDir")).toBool()) {
        // unar exits with code 1 if extraction fails.
        // This happens at least with wrong passwords or not enough space in the destination folder.
        if (m_exitCode == 1) {
            if (password().isEmpty()) {
                qCWarning(ARK) << "Extraction aborted, destination folder might not have enough space.";
                emit error(i18n("Extraction failed. Make sure that enough space is available."));
            } else {
                qCWarning(ARK) << "Extraction aborted, either the password is wrong or the destination folder doesn't have enough space.";
                emit error(i18n("Extraction failed. Make sure you provided the correct password and that enough space is available."));
                setPassword(QString());
            }
            copyProcessCleanup();
            emit finished(false);
            return;
        }

        if (!m_compressionOptions.value(QStringLiteral("DragAndDrop")).toBool()) {
            if (!moveToDestination(QDir::current(), QDir(m_extractDestDir), m_compressionOptions[QStringLiteral("PreservePaths")].toBool())) {
                emit error(i18ncp("@info",
                                  "Could not move the extracted file to the destination directory.",
                                  "Could not move the extracted files to the destination directory.",
                                  m_copiedFiles.size()));
                copyProcessCleanup();
                emit finished(false);
                return;
            }

            copyProcessCleanup();
        }
    }

    if (m_compressionOptions.value(QStringLiteral("DragAndDrop")).toBool()) {
        if (!moveDroppedFilesToDest(m_copiedFiles, m_extractDestDir)) {
            emit error(i18ncp("@info",
                              "Could not move the extracted file to the destination directory.",
                              "Could not move the extracted files to the destination directory.",
                              m_copiedFiles.size()));
            copyProcessCleanup();
            emit finished(false);
            return;
        }

        copyProcessCleanup();
    }

    emit progress(1.0);
    emit finished(true);
}

bool CliInterface::moveDroppedFilesToDest(const QVariantList &files, const QString &finalDest)
{
    // Move extracted files from a QTemporaryDir to the final destination.

    QDir finalDestDir(finalDest);
    qCDebug(ARK) << "Setting final dir to" << finalDest;

    bool overwriteAll = false;
    bool skipAll = false;

    foreach (const QVariant& file, files) {

        QFileInfo relEntry(file.value<fileRootNodePair>().file.remove(file.value<fileRootNodePair>().rootNode));
        QFileInfo absSourceEntry(QDir::current().absolutePath() + QLatin1Char('/') + file.value<fileRootNodePair>().file);
        QFileInfo absDestEntry(finalDestDir.path() + QLatin1Char('/') + relEntry.filePath());

        if (absSourceEntry.isDir()) {

            // For directories, just create the path.
            if (!finalDestDir.mkpath(relEntry.filePath())) {
                qCWarning(ARK) << "Failed to create directory" << relEntry.filePath() << "in final destination.";
            }

        } else {

            // If destination file exists, prompt the user.
            if (absDestEntry.exists()) {
                qCWarning(ARK) << "File" << absDestEntry.absoluteFilePath() << "exists.";

                if (!skipAll && !overwriteAll) {

                    Kerfuffle::OverwriteQuery query(absDestEntry.absoluteFilePath());
                    query.setNoRenameMode(true);
                    emit userQuery(&query);
                    query.waitForResponse();

                    if (query.responseOverwrite() || query.responseOverwriteAll()) {
                        if (query.responseOverwriteAll()) {
                            overwriteAll = true;
                        }
                        if (!QFile::remove(absDestEntry.absoluteFilePath())) {
                            qCWarning(ARK) << "Failed to remove" << absDestEntry.absoluteFilePath();
                        }

                    } else if (query.responseSkip() || query.responseAutoSkip()) {
                        if (query.responseAutoSkip()) {
                            skipAll = true;
                        }
                        continue;

                    } else if (query.responseCancelled()) {
                        qCDebug(ARK) << "Copy action cancelled.";
                        return false;
                    }

                } else if (skipAll) {
                    continue;

                } else if (overwriteAll) {

                    if (!QFile::remove(absDestEntry.absoluteFilePath())) {
                        qCWarning(ARK) << "Failed to remove" << absDestEntry.absoluteFilePath();
                    }
                }

            }

            // Create any parent directories.
            if (!finalDestDir.mkpath(relEntry.path())) {
                qCWarning(ARK) << "Failed to create parent directory for file:" << absDestEntry.filePath();
            }

            // Move files to the final destination.
            if (!QFile(absSourceEntry.absoluteFilePath()).rename(absDestEntry.absoluteFilePath())) {
                qCWarning(ARK) << "Failed to move file" << absSourceEntry.filePath() << "to final destination.";
                return false;
            }
        }
    }
    return true;
}

bool CliInterface::isEmptyDir(const QDir &dir)
{
    QDir d = dir;
    d.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);

    return d.count() == 0;
}

void CliInterface::copyProcessCleanup()
{
    if (!m_oldWorkingDir.isEmpty()) {
        QDir::setCurrent(m_oldWorkingDir);
    }

    if (m_extractTempDir) {
        delete m_extractTempDir;
        m_extractTempDir = Q_NULLPTR;
    }
}

bool CliInterface::moveToDestination(const QDir &tempDir, const QDir &destDir, bool preservePaths)
{
    qCDebug(ARK) << "Moving extracted files from temp dir" << tempDir.path() << "to final destination" << destDir.path();

    bool overwriteAll = false;
    bool skipAll = false;

    QDirIterator dirIt(tempDir.path(), QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (dirIt.hasNext()) {
        dirIt.next();

        // We skip directories if:
        // 1. We are not preserving paths
        // 2. The dir is not empty. Only empty directories need to be explicitly moved.
        // The non-empty ones are created by QDir::mkpath() below.
        if (dirIt.fileInfo().isDir()) {
            if (!preservePaths || !isEmptyDir(QDir(dirIt.filePath()))) {
                continue;
            }
        }

        QFileInfo relEntry;
        if (preservePaths) {
            relEntry = QFileInfo(dirIt.filePath().remove(tempDir.path() + QLatin1Char('/')));
        } else {
            relEntry = QFileInfo(dirIt.fileName());
        }

        QFileInfo absDestEntry(destDir.path() + QLatin1Char('/') + relEntry.filePath());

        if (absDestEntry.exists()) {
            qCWarning(ARK) << "File" << absDestEntry.absoluteFilePath() << "exists.";

            Kerfuffle::OverwriteQuery query(absDestEntry.absoluteFilePath());
            query.setNoRenameMode(true);
            emit userQuery(&query);
            query.waitForResponse();

            if (query.responseOverwrite() || query.responseOverwriteAll()) {
                if (query.responseOverwriteAll()) {
                    overwriteAll = true;
                }
                if (!QFile::remove(absDestEntry.absoluteFilePath())) {
                    qCWarning(ARK) << "Failed to remove" << absDestEntry.absoluteFilePath();
                }

            } else if (query.responseSkip() || query.responseAutoSkip()) {
                if (query.responseAutoSkip()) {
                    skipAll = true;
                }
                continue;
            } else if (query.responseCancelled()) {
                qCDebug(ARK) << "Copy action cancelled.";
                return false;
            }
        } else if (skipAll) {
            continue;
        } else if (overwriteAll) {
            if (!QFile::remove(absDestEntry.absoluteFilePath())) {
                qCWarning(ARK) << "Failed to remove" << absDestEntry.absoluteFilePath();
            }
        }

        if (preservePaths) {
            // Create any parent directories.
            if (!destDir.mkpath(relEntry.path())) {
                qCWarning(ARK) << "Failed to create parent directory for file:" << absDestEntry.filePath();
            }
        }

        // Move file to the final destination.
        if (!QFile(dirIt.filePath()).rename(absDestEntry.absoluteFilePath())) {
            qCWarning(ARK) << "Failed to move file" << dirIt.filePath() << "to final destination.";
            return false;
        }
    }

    return true;
}

QStringList CliInterface::substituteListVariables(const QStringList &listArgs, const QString &password)
{
    // Required if we call this function from unit tests.
    cacheParameterList();

    QStringList args;
    foreach (const QString& arg, listArgs) {
        if (arg == QLatin1String("$Archive")) {
            args << filename();
            continue;
        }

        if (arg == QLatin1String("$PasswordSwitch")) {
            args << passwordSwitch(password);
            continue;
        }

        // Simple argument (e.g. -slt in 7z), nothing to substitute, just add it to the list.
        args << arg;
    }

    // Remove empty strings, if any.
    args.removeAll(QString());

    return args;
}

QStringList CliInterface::substituteCopyVariables(const QStringList &extractArgs, const QVariantList &files, bool preservePaths, const QString &password)
{
    // Required if we call this function from unit tests.
    cacheParameterList();

    QStringList args;
    foreach (const QString& arg, extractArgs) {
        qCDebug(ARK) << "Processing argument" << arg;

        if (arg == QLatin1String("$Archive")) {
            args << filename();
            continue;
        }

        if (arg == QLatin1String("$PreservePathSwitch")) {
            args << preservePathSwitch(preservePaths);
            continue;
        }

        if (arg == QLatin1String("$PasswordSwitch")) {
            args << passwordSwitch(password);
            continue;
        }

        if (arg == QLatin1String("$Files")) {
            args << copyFilesList(files);
            continue;
        }

        // Simple argument (e.g. -kb in unrar), nothing to substitute, just add it to the list.
        args << arg;
    }

    // Remove empty strings, if any.
    args.removeAll(QString());

    return args;
}

QStringList CliInterface::substituteAddVariables(const QStringList &addArgs, const QStringList &files, const QString &password, bool encryptHeader, int compLevel, ulong volumeSize)
{
    // Required if we call this function from unit tests.
    cacheParameterList();

    QStringList args;
    foreach (const QString& arg, addArgs) {
        qCDebug(ARK) << "Processing argument " << arg;

        if (arg == QLatin1String("$Archive")) {
            args << filename();
            continue;
        }

        if (arg == QLatin1String("$PasswordSwitch")) {
            args << (encryptHeader ? passwordHeaderSwitch(password) : passwordSwitch(password));
            continue;
        }

        if (arg == QLatin1String("$CompressionLevelSwitch")) {
            args << compressionLevelSwitch(compLevel);
            continue;
        }

        if (arg == QLatin1String("$MultiVolumeSwitch")) {
            args << multiVolumeSwitch(volumeSize);
            continue;
        }

        if (arg == QLatin1String("$Files")) {
            args << files;
            continue;
        }

        // Simple argument (e.g. a in 7z), nothing to substitute, just add it to the list.
        args << arg;
    }

    // Remove empty strings, if any.
    args.removeAll(QString());

    return args;
}

QStringList CliInterface::substituteDeleteVariables(const QStringList &deleteArgs, const QVariantList &files, const QString &password)
{
    cacheParameterList();

    QStringList args;
    foreach (const QString& arg, deleteArgs) {
        qCDebug(ARK) << "Processing argument" << arg;

        if (arg == QLatin1String("$Archive")) {
            args << filename();
            continue;
        }

        if (arg == QLatin1String("$PasswordSwitch")) {
            args << passwordSwitch(password);
            continue;
        }

        if (arg == QLatin1String("$Files")) {
            foreach (const QVariant& file, files) {
                args << escapeFileName(file.toString());
            }
            continue;
        }

        // Simple argument (e.g. d in rar), nothing to substitute, just add it to the list.
        args << arg;
    }

    // Remove empty strings, if any.
    args.removeAll(QString());

    return args;
}

QStringList CliInterface::substituteCommentVariables(const QStringList &commentArgs, const QString &commentFile)
{
    // Required if we call this function from unit tests.
    cacheParameterList();

    QStringList args;
    foreach (const QString& arg, commentArgs) {
        qCDebug(ARK) << "Processing argument " << arg;

        if (arg == QLatin1String("$Archive")) {
            args << filename();
            continue;
        }

        if (arg == QLatin1String("$CommentSwitch")) {
            QString commentSwitch = m_param.value(CommentSwitch).toString();
            commentSwitch.replace(QStringLiteral("$CommentFile"), commentFile);
            args << commentSwitch;
            continue;
        }

        args << arg;
    }

    // Remove empty strings, if any.
    args.removeAll(QString());

    return args;
}

QStringList CliInterface::substituteTestVariables(const QStringList &testArgs, const QString &password)
{
    // Required if we call this function from unit tests.
    cacheParameterList();

    QStringList args;
    foreach (const QString& arg, testArgs) {
        qCDebug(ARK) << "Processing argument " << arg;

        if (arg == QLatin1String("$Archive")) {
            args << filename();
            continue;
        }

        if (arg == QLatin1String("$PasswordSwitch")) {
            args << passwordSwitch(password);
            continue;
        }

        args << arg;
    }

    // Remove empty strings, if any.
    args.removeAll(QString());

    return args;
}

QString CliInterface::preservePathSwitch(bool preservePaths) const
{
    Q_ASSERT(m_param.contains(PreservePathSwitch));
    const QStringList theSwitch = m_param.value(PreservePathSwitch).toStringList();
    Q_ASSERT(theSwitch.size() == 2);

    return (preservePaths ? theSwitch.at(0) : theSwitch.at(1));
}

QStringList CliInterface::passwordHeaderSwitch(const QString& password) const
{
    if (password.isEmpty()) {
        return QStringList();
    }

    Q_ASSERT(m_param.contains(PasswordHeaderSwitch));

    QStringList passwordHeaderSwitch = m_param.value(PasswordHeaderSwitch).toStringList();
    Q_ASSERT(!passwordHeaderSwitch.isEmpty() && passwordHeaderSwitch.size() <= 2);

    if (passwordHeaderSwitch.size() == 1) {
        passwordHeaderSwitch[0].replace(QLatin1String("$Password"), password);
    } else {
        passwordHeaderSwitch[1] = password;
    }

    return passwordHeaderSwitch;
}

QStringList CliInterface::passwordSwitch(const QString& password) const
{
    if (password.isEmpty()) {
        return QStringList();
    }

    Q_ASSERT(m_param.contains(PasswordSwitch));

    QStringList passwordSwitch = m_param.value(PasswordSwitch).toStringList();
    Q_ASSERT(!passwordSwitch.isEmpty() && passwordSwitch.size() <= 2);

    if (passwordSwitch.size() == 1) {
        passwordSwitch[0].replace(QLatin1String("$Password"), password);
    } else {
        passwordSwitch[1] = password;
    }

    return passwordSwitch;
}

QString CliInterface::compressionLevelSwitch(int level) const
{
    if (level < 0 || level > 9) {
        return QString();
    }

    Q_ASSERT(m_param.contains(CompressionLevelSwitch));

    QString compLevelSwitch = m_param.value(CompressionLevelSwitch).toString();
    Q_ASSERT(!compLevelSwitch.isEmpty());

    compLevelSwitch.replace(QLatin1String("$CompressionLevel"), QString::number(level));

    return compLevelSwitch;
}

QString CliInterface::multiVolumeSwitch(ulong volumeSize) const
{
    // The maximum value we allow in the QDoubleSpinBox is 1000MB. Converted to
    // KB this is 1024000.
    if (volumeSize <= 0 || volumeSize > 1024000) {
        return QString();
    }

    Q_ASSERT(m_param.contains(MultiVolumeSwitch));

    QString multiVolumeSwitch = m_param.value(MultiVolumeSwitch).toString();
    Q_ASSERT(!multiVolumeSwitch.isEmpty());

    multiVolumeSwitch.replace(QLatin1String("$VolumeSize"), QString::number(volumeSize));

    return multiVolumeSwitch;
}

QStringList CliInterface::copyFilesList(const QVariantList& files) const
{
    QStringList filesList;
    foreach (const QVariant& f, files) {
        filesList << escapeFileName(f.value<fileRootNodePair>().file);
    }

    return filesList;
}

void CliInterface::killProcess(bool emitFinished)
{
    // TODO: Would be good to unit test #304764/#304178.

    if (!m_process) {
        return;
    }

    m_abortingOperation = !emitFinished;

    // Give some time for the application to finish gracefully
    if (!m_process->waitForFinished(5)) {
        m_process->kill();

        // It takes a few hundred ms for the process to be killed.
        m_process->waitForFinished(1000);
    }

    m_abortingOperation = false;
}

bool CliInterface::passwordQuery()
{
    Kerfuffle::PasswordNeededQuery query(filename());
    emit userQuery(&query);
    query.waitForResponse();

    if (query.responseCancelled()) {
        emit cancelled();
        // There is no process running, so finished() must be emitted manually.
        emit finished(false);
        return false;
    }

    setPassword(query.password());
    return true;
}

void CliInterface::readStdout(bool handleAll)
{
    //when hacking this function, please remember the following:
    //- standard output comes in unpredictable chunks, this is why
    //you can never know if the last part of the output is a complete line or not
    //- console applications are not really consistent about what
    //characters they send out (newline, backspace, carriage return,
    //etc), so keep in mind that this function is supposed to handle
    //all those special cases and be the lowest common denominator

    if (m_abortingOperation)
        return;

    Q_ASSERT(m_process);

    if (!m_process->bytesAvailable()) {
        //if process has no more data, we can just bail out
        return;
    }

    QByteArray dd = m_process->readAllStandardOutput();
    m_stdOutData += dd;

    QList<QByteArray> lines = m_stdOutData.split('\n');

    //The reason for this check is that archivers often do not end
    //queries (such as file exists, wrong password) on a new line, but
    //freeze waiting for input. So we check for errors on the last line in
    //all cases.
    // TODO: QLatin1String() might not be the best choice here.
    //       The call to handleLine() at the end of the method uses
    //       QString::fromLocal8Bit(), for example.
    // TODO: The same check methods are called in handleLine(), this
    //       is suboptimal.

    bool wrongPasswordMessage = checkForErrorMessage(QLatin1String( lines.last() ), WrongPasswordPatterns);

    bool foundErrorMessage =
        (wrongPasswordMessage ||
         checkForErrorMessage(QLatin1String(lines.last()), DiskFullPatterns) ||
         checkForErrorMessage(QLatin1String(lines.last()), ExtractionFailedPatterns) ||
         checkForPasswordPromptMessage(QLatin1String(lines.last())) ||
         checkForErrorMessage(QLatin1String(lines.last()), FileExistsExpression));

    if (foundErrorMessage) {
        handleAll = true;
    }

    if (wrongPasswordMessage) {
        setPassword(QString());
    }

    //this is complex, here's an explanation:
    //if there is no newline, then there is no guaranteed full line to
    //handle in the output. The exception is that it is supposed to handle
    //all the data, OR if there's been an error message found in the
    //partial data.
    if (lines.size() == 1 && !handleAll) {
        return;
    }

    if (handleAll) {
        m_stdOutData.clear();
    } else {
        //because the last line might be incomplete we leave it for now
        //note, this last line may be an empty string if the stdoutdata ends
        //with a newline
        m_stdOutData = lines.takeLast();
    }

    foreach(const QByteArray& line, lines) {
        if (!line.isEmpty() || (m_listEmptyLines && m_operationMode == List)) {
            if (!handleLine(QString::fromLocal8Bit(line))) {
                killProcess();
                return;
            }
        }
    }
}

bool CliInterface::handleLine(const QString& line)
{
    // TODO: This should be implemented by each plugin; the way progress is
    //       shown by each CLI application is subject to a lot of variation.
    if ((m_operationMode == Copy || m_operationMode == Add) && m_param.contains(CaptureProgress) && m_param.value(CaptureProgress).toBool()) {
        //read the percentage
        int pos = line.indexOf(QLatin1Char( '%' ));
        if (pos > 1) {
            int percentage = line.midRef(pos - 2, 2).toInt();
            emit progress(float(percentage) / 100);
            return true;
        }
    }

    if (m_operationMode == Copy) {

        if (checkForPasswordPromptMessage(line)) {
            qCDebug(ARK) << "Found a password prompt";

            Kerfuffle::PasswordNeededQuery query(filename());
            emit userQuery(&query);
            query.waitForResponse();

            if (query.responseCancelled()) {
                emit cancelled();
                return false;
            }

            setPassword(query.password());

            const QString response(password() + QLatin1Char('\n'));
            writeToProcess(response.toLocal8Bit());

            return true;
        }

        if (checkForErrorMessage(line, DiskFullPatterns)) {
            qCWarning(ARK) << "Found disk full message:" << line;
            emit error(i18nc("@info", "Extraction failed because the disk is full."));
            return false;
        }

        if (checkForErrorMessage(line, WrongPasswordPatterns)) {
            qCWarning(ARK) << "Wrong password!";
            setPassword(QString());
            emit error(i18nc("@info", "Extraction failed: Incorrect password"));
            return false;
        }

        if (checkForErrorMessage(line, ExtractionFailedPatterns)) {
            qCWarning(ARK) << "Error in extraction:" << line;
            emit error(i18n("Extraction failed because of an unexpected error."));
            return false;
        }

        if (handleFileExistsMessage(line)) {
            return true;
        }
    }

    if (m_operationMode == List) {
        if (checkForPasswordPromptMessage(line)) {
            qCDebug(ARK) << "Found a password prompt";

            Kerfuffle::PasswordNeededQuery query(filename());
            emit userQuery(&query);
            query.waitForResponse();

            if (query.responseCancelled()) {
                emit cancelled();
                return false;
            }

            setPassword(query.password());

            const QString response(password() + QLatin1Char('\n'));
            writeToProcess(response.toLocal8Bit());

            return true;
        }

        if (checkForErrorMessage(line, WrongPasswordPatterns)) {
            qCWarning(ARK) << "Wrong password!";
            setPassword(QString());
            emit error(i18n("Incorrect password."));
            return false;
        }

        if (checkForErrorMessage(line, ExtractionFailedPatterns)) {
            qCWarning(ARK) << "Error in extraction!!";
            emit error(i18n("Extraction failed because of an unexpected error."));
            return false;
        }

        if (checkForErrorMessage(line, CorruptArchivePatterns)) {
            qCWarning(ARK) << "Archive corrupt";
            setCorrupt(true);
            // Special case: corrupt is not a "fatal" error so we return true here.
            return true;
        }

        if (handleFileExistsMessage(line)) {
            return true;
        }

        readListLine(line);
        return true;
    }

    if (m_operationMode == Test) {

        if (checkForPasswordPromptMessage(line)) {
            qCDebug(ARK) << "Found a password prompt";

            emit error(i18n("Ark does not currently support testing this archive."));
            return false;
        }

        if (checkForTestSuccessMessage(line)) {
            qCDebug(ARK) << "Test successful";
            emit testSuccess();
            return true;
        }
    }

    return true;
}

bool CliInterface::checkForPasswordPromptMessage(const QString& line)
{
    const QString passwordPromptPattern(m_param.value(PasswordPromptPattern).toString());

    if (passwordPromptPattern.isEmpty())
        return false;

    if (m_passwordPromptPattern.pattern().isEmpty()) {
        m_passwordPromptPattern.setPattern(m_param.value(PasswordPromptPattern).toString());
    }

    if (m_passwordPromptPattern.match(line).hasMatch()) {
        return true;
    }

    return false;
}

bool CliInterface::handleFileExistsMessage(const QString& line)
{
    // Check for a filename and store it.
    foreach (const QString &pattern, m_param.value(FileExistsFileName).toStringList()) {
        const QRegularExpression rxFileNamePattern(pattern);
        const QRegularExpressionMatch rxMatch = rxFileNamePattern.match(line);

        if (rxMatch.hasMatch()) {
            m_storedFileName = rxMatch.captured(1);
            qCWarning(ARK) << "Detected existing file:" << m_storedFileName;
        }
    }

    if (!checkForErrorMessage(line, FileExistsExpression)) {
        return false;
    }

    Kerfuffle::OverwriteQuery query(QDir::current().path() + QLatin1Char( '/' ) + m_storedFileName);
    query.setNoRenameMode(true);
    emit userQuery(&query);
    qCDebug(ARK) << "Waiting response";
    query.waitForResponse();

    qCDebug(ARK) << "Finished response";

    QString responseToProcess;
    const QStringList choices = m_param.value(FileExistsInput).toStringList();

    if (query.responseOverwrite()) {
        responseToProcess = choices.at(0);
    } else if (query.responseSkip()) {
        responseToProcess = choices.at(1);
    } else if (query.responseOverwriteAll()) {
        responseToProcess = choices.at(2);
    } else if (query.responseAutoSkip()) {
        responseToProcess = choices.at(3);
    } else if (query.responseCancelled()) {
        if (choices.count() < 5) { // If the program has no way to cancel the extraction, we resort to killing it
            return doKill();
        }
        responseToProcess = choices.at(4);
    }

    Q_ASSERT(!responseToProcess.isEmpty());

    responseToProcess += QLatin1Char( '\n' );

    writeToProcess(responseToProcess.toLocal8Bit());

    return true;
}

bool CliInterface::checkForErrorMessage(const QString& line, int parameterIndex)
{
    QList<QRegularExpression> patterns;

    if (m_patternCache.contains(parameterIndex)) {
        patterns = m_patternCache.value(parameterIndex);
    } else {
        if (!m_param.contains(parameterIndex)) {
            return false;
        }

        foreach(const QString& rawPattern, m_param.value(parameterIndex).toStringList()) {
            patterns << QRegularExpression(rawPattern);
        }
        m_patternCache[parameterIndex] = patterns;
    }

    foreach(const QRegularExpression& pattern, patterns) {
        if (pattern.match(line).hasMatch()) {
            return true;
        }
    }
    return false;
}

bool CliInterface::checkForTestSuccessMessage(const QString& line)
{
    const QRegularExpression rx(m_param.value(TestPassedPattern).toString());
    const QRegularExpressionMatch rxMatch = rx.match(line);
    if (rxMatch.hasMatch()) {
        return true;
    }
    return false;
}

bool CliInterface::doKill()
{
    if (m_process) {
        killProcess(false);
        return true;
    }

    return false;
}

bool CliInterface::doSuspend()
{
    return false;
}

bool CliInterface::doResume()
{
    return false;
}

QString CliInterface::escapeFileName(const QString& fileName) const
{
    return fileName;
}

void CliInterface::writeToProcess(const QByteArray& data)
{
    Q_ASSERT(m_process);
    Q_ASSERT(!data.isNull());

    qCDebug(ARK) << "Writing" << data << "to the process";

#ifdef Q_OS_WIN
    m_process->write(data);
#else
    m_process->pty()->write(data);
#endif
}

bool CliInterface::addComment(const QString &comment)
{
    cacheParameterList();

    m_operationMode = Comment;

    m_commentTempFile = new QTemporaryFile;
    if (!m_commentTempFile->open()) {
        qCWarning(ARK) << "Failed to create temporary file for comment";
        emit finished(false);
        return false;
    }

    QTextStream stream(m_commentTempFile);
    stream << comment << endl;
    m_commentTempFile->close();

    const auto args = substituteCommentVariables(m_param.value(CommentArgs).toStringList(),
                                                 m_commentTempFile->fileName());

    if (!runProcess(m_param.value(AddProgram).toStringList(), args)) {
        return false;
    }
    m_comment = comment;
    return true;
}

QString CliInterface::multiVolumeName() const
{
    QString oldSuffix = QMimeDatabase().suffixForFileName(filename());
    QString name;

    foreach (const QString &multiSuffix, m_param.value(MultiVolumeSuffix).toStringList()) {
        QString newSuffix = multiSuffix;
        newSuffix.replace(QStringLiteral("$Suffix"), oldSuffix);
        name = filename().remove(oldSuffix).append(newSuffix);
        if (QFileInfo::exists(name)) {
            break;
        }
    }
    return name;
}

}

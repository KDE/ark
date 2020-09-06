/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (c) 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
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

#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTemporaryFile>
#include <QThread>
#include <QUrl>
#include <QCoreApplication>

namespace Kerfuffle
{
CliInterface::CliInterface(QObject *parent, const QVariantList & args)
    : ReadWriteArchiveInterface(parent, args)
{
    //because this interface uses the event loop
    setWaitForFinishedSignal(true);

    if (QMetaType::type("QProcess::ExitStatus") == 0) {
        qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
    }
    m_cliProps = new CliProperties(this, m_metaData, mimetype());
}

CliInterface::~CliInterface()
{
    Q_ASSERT(!m_process);
}

void CliInterface::setListEmptyLines(bool emptyLines)
{
    m_listEmptyLines = emptyLines;
}

int CliInterface::copyRequiredSignals() const
{
    return 2;
}

bool CliInterface::list()
{
    resetParsing();
    m_operationMode = List;
    m_numberOfEntries = 0;

    // To compute progress.
    m_archiveSizeOnDisk = static_cast<qulonglong>(QFileInfo(filename()).size());
    connect(this, &ReadOnlyArchiveInterface::entry, this, &CliInterface::onEntry);

    return runProcess(m_cliProps->property("listProgram").toString(), m_cliProps->listArgs(filename(), password()));
}

bool CliInterface::extractFiles(const QVector<Archive::Entry*> &files, const QString &destinationDirectory, const ExtractionOptions &options)
{
    qCDebug(ARK) << "destination directory:" << destinationDirectory;

    m_operationMode = Extract;
    m_extractionOptions = options;
    m_extractedFiles = files;
    m_extractDestDir = destinationDirectory;



    if (!m_cliProps->property("passwordSwitch").toStringList().isEmpty() && options.encryptedArchiveHint() && password().isEmpty()) {
        qCDebug(ARK) << "Password hint enabled, querying user";
        if (!passwordQuery()) {
            return false;
        }
    }

    QUrl destDir = QUrl(destinationDirectory);
    m_oldWorkingDirExtraction = QDir::currentPath();
    QDir::setCurrent(destDir.adjusted(QUrl::RemoveScheme).url());

    const bool useTmpExtractDir = options.isDragAndDropEnabled() || options.alwaysUseTempDir();

    if (useTmpExtractDir) {
        // Create an hidden temp folder in the current directory.
        m_extractTempDir.reset(new QTemporaryDir(QStringLiteral(".%1-").arg(QCoreApplication::applicationName())));

        qCDebug(ARK) << "Using temporary extraction dir:" << m_extractTempDir->path();
        if (!m_extractTempDir->isValid()) {
            qCDebug(ARK) << "Creation of temporary directory failed.";
            Q_EMIT finished(false);
            return false;
        }
        destDir = QUrl(m_extractTempDir->path());
        QDir::setCurrent(destDir.adjusted(QUrl::RemoveScheme).url());
    }

    return runProcess(m_cliProps->property("extractProgram").toString(),
                    m_cliProps->extractArgs(filename(),
                                            extractFilesList(files),
                                            options.preservePaths(),
                                            password()));
}

bool CliInterface::addFiles(const QVector<Archive::Entry*> &files, const Archive::Entry *destination, const CompressionOptions& options, uint numberOfEntriesToAdd)
{
    Q_UNUSED(numberOfEntriesToAdd)

    m_operationMode = Add;

    QVector<Archive::Entry*> filesToPass = QVector<Archive::Entry*>();
    // If destination path is specified, we have recreate its structure inside the temp directory
    // and then place symlinks of targeted files there.
    const QString destinationPath = (destination == nullptr)
                                    ? QString()
                                    : destination->fullPath();

    qCDebug(ARK) << "Adding" << files.count() << "file(s) to destination:" << destinationPath;

    if (!destinationPath.isEmpty()) {
        m_extractTempDir.reset(new QTemporaryDir());
        const QString absoluteDestinationPath = m_extractTempDir->path() + QLatin1Char('/') + destinationPath;

        QDir qDir;
        qDir.mkpath(absoluteDestinationPath);

        QObject *preservedParent = nullptr;
        for (Archive::Entry *file : files) {
            // The entries may have parent. We have to save and apply it to our new entry in order to prevent memory
            // leaks.
            if (preservedParent == nullptr) {
                preservedParent = file->parent();
            }

            const QString filePath = QDir::currentPath() + QLatin1Char('/') + file->fullPath(NoTrailingSlash);
            const QString newFilePath = absoluteDestinationPath + file->fullPath(NoTrailingSlash);
            if (QFile::link(filePath, newFilePath)) {
                qCDebug(ARK) << "Symlink's created:" << filePath << newFilePath;
            } else {
                qCDebug(ARK) << "Can't create symlink" << filePath << newFilePath;
                Q_EMIT finished(false);
                return false;
            }
        }

        qCDebug(ARK) << "Changing working dir again to " << m_extractTempDir->path();
        QDir::setCurrent(m_extractTempDir->path());

#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
        filesToPass.push_back(new Archive::Entry(preservedParent, destinationPath.split(QLatin1Char('/'), QString::SkipEmptyParts).at(0)));
#else
        filesToPass.push_back(new Archive::Entry(preservedParent, destinationPath.split(QLatin1Char('/'), Qt::SkipEmptyParts).at(0)));
#endif
    } else {
        filesToPass = files;
    }

    if (!m_cliProps->property("passwordSwitch").toString().isEmpty() && options.encryptedArchiveHint() && password().isEmpty()) {
        qCDebug(ARK) << "Password hint enabled, querying user";
        if (!passwordQuery()) {
            return false;
        }
    }

    return runProcess(m_cliProps->property("addProgram").toString(),
                      m_cliProps->addArgs(filename(),
                                          entryFullPaths(filesToPass, NoTrailingSlash),
                                          password(),
                                          isHeaderEncryptionEnabled(),
                                          options.compressionLevel(),
                                          options.compressionMethod(),
                                          options.encryptionMethod(),
                                          options.volumeSize()));
}

bool CliInterface::moveFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    Q_UNUSED(options);

    m_operationMode = Move;

    m_removedFiles = files;
    QVector<Archive::Entry*> withoutChildren = entriesWithoutChildren(files);
    setNewMovedFiles(files, destination, withoutChildren.count());

    return runProcess(m_cliProps->property("moveProgram").toString(),
                      m_cliProps->moveArgs(filename(),
                                           withoutChildren,
                                           destination,
                                           password()));
}

bool CliInterface::copyFiles(const QVector<Archive::Entry*> &files, Archive::Entry *destination, const CompressionOptions &options)
{
    m_oldWorkingDir = QDir::currentPath();
    m_tempWorkingDir.reset(new QTemporaryDir());
    m_tempAddDir.reset(new QTemporaryDir());
    QDir::setCurrent(m_tempWorkingDir->path());
    m_passedFiles = files;
    m_passedDestination = destination;
    m_passedOptions = options;
    m_numberOfEntries = 0;

    m_subOperation = Extract;
    connect(this, &CliInterface::finished, this, &CliInterface::continueCopying);

    return extractFiles(files, QDir::currentPath(), ExtractionOptions());
}

bool CliInterface::deleteFiles(const QVector<Archive::Entry*> &files)
{
    m_operationMode = Delete;

    m_removedFiles = files;

    return runProcess(m_cliProps->property("deleteProgram").toString(),
                      m_cliProps->deleteArgs(filename(), files, password()));
}

bool CliInterface::testArchive()
{
    resetParsing();
    m_operationMode = Test;

    return runProcess(m_cliProps->property("testProgram").toString(), m_cliProps->testArgs(filename(), password()));
}

bool CliInterface::runProcess(const QString& programName, const QStringList& arguments)
{
    Q_ASSERT(!m_process);

    QString programPath = QStandardPaths::findExecutable(programName);
    if (programPath.isEmpty()) {
        Q_EMIT error(xi18nc("@info", "Failed to locate program <filename>%1</filename> on disk.", programName));
        Q_EMIT finished(false);
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

    m_readyStdOutConnection = connect(m_process, &QProcess::readyReadStandardOutput, this, [=]() {
        readStdout();
    });

    if (m_operationMode == Extract) {
        // Extraction jobs need a dedicated post-processing function.
        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &CliInterface::extractProcessFinished);
    } else {
        connect(m_process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &CliInterface::processFinished);
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
        m_process = nullptr;
    }

    // #193908 - #222392
    // Don't emit finished() if the job was killed quietly.
    if (m_abortingOperation) {
        return;
    }

    if (m_operationMode == Delete || m_operationMode == Move) {
        const QStringList removedFullPaths = entryFullPaths(m_removedFiles);
        for (const QString &fullPath : removedFullPaths) {
            Q_EMIT entryRemoved(fullPath);
        }
        for (Archive::Entry *e : qAsConst(m_newMovedFiles)) {
            Q_EMIT entry(e);
        }
        m_newMovedFiles.clear();
    }

    if (m_operationMode == Add && !isMultiVolume()) {
        list();
    } else if (m_operationMode == List && isCorrupt()) {
        Kerfuffle::LoadCorruptQuery query(filename());
        query.execute();
        if (!query.responseYes()) {
            Q_EMIT cancelled();
            Q_EMIT finished(false);
        } else {
            Q_EMIT progress(1.0);
            Q_EMIT finished(true);
        }
    } else  {
        Q_EMIT progress(1.0);
        Q_EMIT finished(true);
    }
}

void CliInterface::extractProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    Q_ASSERT(m_operationMode == Extract);

    m_exitCode = exitCode;
    qCDebug(ARK) << "Extraction process finished, exitcode:" << exitCode << "exitstatus:" << exitStatus;

    if (m_process) {
        // Handle all the remaining data in the process.
        readStdout(true);

        delete m_process;
        m_process = nullptr;
    }

    // Don't emit finished() if the job was killed quietly.
    if (m_abortingOperation) {
        return;
    }

    if (m_extractionOptions.alwaysUseTempDir()) {
        // unar exits with code 1 if extraction fails.
        // This happens at least with wrong passwords or not enough space in the destination folder.
        if (m_exitCode == 1) {
            if (password().isEmpty()) {
                qCWarning(ARK) << "Extraction aborted, destination folder might not have enough space.";
                Q_EMIT error(i18n("Extraction failed. Make sure that enough space is available."));
            } else {
                qCWarning(ARK) << "Extraction aborted, either the password is wrong or the destination folder doesn't have enough space.";
                Q_EMIT error(i18n("Extraction failed. Make sure you provided the correct password and that enough space is available."));
                setPassword(QString());
            }
            cleanUpExtracting();
            Q_EMIT finished(false);
            return;
        }

        if (!m_extractionOptions.isDragAndDropEnabled()) {
            if (!moveToDestination(QDir::current(), QDir(m_extractDestDir), m_extractionOptions.preservePaths())) {
                Q_EMIT error(i18ncp("@info",
                                  "Could not move the extracted file to the destination directory.",
                                  "Could not move the extracted files to the destination directory.",
                                  m_extractedFiles.size()));
                cleanUpExtracting();
                Q_EMIT finished(false);
                return;
            }

            cleanUpExtracting();
        }
    }

    if (m_extractionOptions.isDragAndDropEnabled()) {
        const bool droppedFilesMoved = moveDroppedFilesToDest(m_extractedFiles, m_extractDestDir);
        if (!droppedFilesMoved) {
            cleanUpExtracting();
            return;
        }

        cleanUpExtracting();
    }

    // #395939: make sure we *always* restore the old working dir.
    restoreWorkingDirExtraction();

    Q_EMIT progress(1.0);
    Q_EMIT finished(true);
}

void CliInterface::continueCopying(bool result)
{
    if (!result) {
        finishCopying(false);
        return;
    }

    switch (m_subOperation) {
    case Extract:
        m_subOperation = Add;
        m_passedFiles = entriesWithoutChildren(m_passedFiles);
        if (!setAddedFiles() || !addFiles(m_tempAddedFiles, m_passedDestination, m_passedOptions)) {
            finishCopying(false);
        }
        break;
    case Add:
        finishCopying(true);
        break;
    default:
        Q_ASSERT(false);
    }
}

bool CliInterface::moveDroppedFilesToDest(const QVector<Archive::Entry*> &files, const QString &finalDest)
{
    // Move extracted files from a QTemporaryDir to the final destination.

    QDir finalDestDir(finalDest);
    qCDebug(ARK) << "Setting final dir to" << finalDest;

    bool overwriteAll = false;
    bool skipAll = false;

    for (const Archive::Entry *file : files) {

        QFileInfo relEntry(file->fullPath().remove(file->rootNode));
        QFileInfo absSourceEntry(QDir::current().absolutePath() + QLatin1Char('/') + file->fullPath());
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
                    query.execute();

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
                        Q_EMIT cancelled();
                        Q_EMIT finished(false);
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
                Q_EMIT error(i18ncp("@info",
                                  "Could not move the extracted file to the destination directory.",
                                  "Could not move the extracted files to the destination directory.",
                                  m_extractedFiles.size()));
                Q_EMIT finished(false);
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

void CliInterface::cleanUpExtracting()
{
    restoreWorkingDirExtraction();
    m_extractTempDir.reset();
}

void CliInterface::restoreWorkingDirExtraction()
{
    if (m_oldWorkingDirExtraction.isEmpty()) {
        return;
    }

    if (!QDir::setCurrent(m_oldWorkingDirExtraction)) {
        qCWarning(ARK) << "Failed to restore old working directory:" << m_oldWorkingDirExtraction;
    } else {
        m_oldWorkingDirExtraction.clear();
    }
}

void CliInterface::finishCopying(bool result)
{
    disconnect(this, &CliInterface::finished, this, &CliInterface::continueCopying);
    Q_EMIT progress(1.0);
    Q_EMIT finished(result);
    cleanUp();
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
            query.execute();

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

void CliInterface::setNewMovedFiles(const QVector<Archive::Entry*> &entries, const Archive::Entry *destination, int entriesWithoutChildren)
{
    m_newMovedFiles.clear();
    QMap<QString, const Archive::Entry*> entryMap;
    for (const Archive::Entry* entry : entries) {
        entryMap.insert(entry->fullPath(), entry);
    }

    QString lastFolder;

    QString newPath;
    int nameLength = 0;
    for (const Archive::Entry* entry : qAsConst(entryMap)) {
        if (lastFolder.count() > 0 && entry->fullPath().startsWith(lastFolder)) {
            // Replace last moved or copied folder path with destination path.
            int charsCount = entry->fullPath().count() - lastFolder.count();
            if (entriesWithoutChildren > 1) {
                charsCount += nameLength;
            }
            newPath = destination->fullPath() + entry->fullPath().right(charsCount);
        } else {
            if (entriesWithoutChildren > 1) {
                newPath = destination->fullPath() + entry->name();
            } else {
                // If there is only one passed file in the list,
                // we have to use destination as newPath.
                newPath = destination->fullPath(NoTrailingSlash);
            }
            if (entry->isDir()) {
                newPath += QLatin1Char('/');
                nameLength = entry->name().count() + 1; // plus slash
                lastFolder = entry->fullPath();
            } else {
                nameLength = 0;
                lastFolder = QString();
            }
        }
        Archive::Entry *newEntry = new Archive::Entry(nullptr);
        newEntry->copyMetaData(entry);
        newEntry->setFullPath(newPath);
        m_newMovedFiles << newEntry;
    }
}

QStringList CliInterface::extractFilesList(const QVector<Archive::Entry*> &entries) const
{
    QStringList filesList;
    for (const Archive::Entry *e : entries) {
        filesList << escapeFileName(e->fullPath(NoTrailingSlash));
    }

    return filesList;
}

void CliInterface::killProcess(bool emitFinished)
{
    // TODO: Would be good to unit test #304764/#304178.

    if (!m_process) {
        return;
    }

    // waitForFinished() will enter QT's event loop. Disconnect the readyReadStandardOutput
    // signal, to avoid an endless recursion, if the process keeps spamming output on stdout.
    disconnect(m_readyStdOutConnection);

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
    query.execute();

    if (query.responseCancelled()) {
        Q_EMIT cancelled();
        // There is no process running, so finished() must be emitted manually.
        Q_EMIT finished(false);
        return false;
    }

    setPassword(query.password());
    return true;
}

void CliInterface::cleanUp()
{
    qDeleteAll(m_tempAddedFiles);
    m_tempAddedFiles.clear();
    QDir::setCurrent(m_oldWorkingDir);
    m_tempWorkingDir.reset();
    m_tempAddDir.reset();
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

    bool wrongPasswordMessage = isWrongPasswordMsg(QLatin1String(lines.last()));

    bool foundErrorMessage =
        (wrongPasswordMessage ||
         isDiskFullMsg(QLatin1String(lines.last())) ||
         isFileExistsMsg(QLatin1String(lines.last()))) ||
         isPasswordPrompt(QLatin1String(lines.last()));

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

    for (const QByteArray& line : qAsConst(lines)) {
        if (!line.isEmpty() || (m_listEmptyLines && m_operationMode == List)) {
            if (!handleLine(QString::fromLocal8Bit(line))) {
                killProcess();
                return;
            }
        }
    }
}

bool CliInterface::setAddedFiles()
{
    QDir::setCurrent(m_tempAddDir->path());
    for (const Archive::Entry *file : qAsConst(m_passedFiles)) {
        const QString oldPath = m_tempWorkingDir->path() + QLatin1Char('/') + file->fullPath(NoTrailingSlash);
        const QString newPath = m_tempAddDir->path() + QLatin1Char('/') + file->name();
        if (!QFile::rename(oldPath, newPath)) {
            return false;
        }
        m_tempAddedFiles << new Archive::Entry(nullptr, file->name());
    }
    return true;
}

bool CliInterface::handleLine(const QString& line)
{
    // TODO: This should be implemented by each plugin; the way progress is
    //       shown by each CLI application is subject to a lot of variation.
    if ((m_operationMode == Extract || m_operationMode == Add) && m_cliProps->property("captureProgress").toBool()) {
        //read the percentage
        int pos = line.indexOf(QLatin1Char( '%' ));
        if (pos > 1) {
            int percentage = line.midRef(pos - 2, 2).toInt();
            Q_EMIT progress(float(percentage) / 100);
            return true;
        }
    }

    if (m_operationMode == Extract) {

        if (isPasswordPrompt(line)) {
            qCDebug(ARK) << "Found a password prompt";

            Kerfuffle::PasswordNeededQuery query(filename());
            query.execute();

            if (query.responseCancelled()) {
                Q_EMIT cancelled();
                return false;
            }

            setPassword(query.password());

            const QString response(password() + QLatin1Char('\n'));
            writeToProcess(response.toLocal8Bit());

            return true;
        }

        if (isDiskFullMsg(line)) {
            qCWarning(ARK) << "Found disk full message:" << line;
            Q_EMIT error(i18nc("@info", "Extraction failed because the disk is full."));
            return false;
        }

        if (isWrongPasswordMsg(line)) {
            qCWarning(ARK) << "Wrong password!";
            setPassword(QString());
            Q_EMIT error(i18nc("@info", "Extraction failed: Incorrect password"));
            return false;
        }

        if (handleFileExistsMessage(line)) {
            return true;
        }

        return readExtractLine(line);
    }

    if (m_operationMode == List) {
        if (isPasswordPrompt(line)) {
            qCDebug(ARK) << "Found a password prompt";

            Kerfuffle::PasswordNeededQuery query(filename());
            query.execute();

            if (query.responseCancelled()) {
                Q_EMIT cancelled();
                return false;
            }

            setPassword(query.password());

            const QString response(password() + QLatin1Char('\n'));
            writeToProcess(response.toLocal8Bit());

            return true;
        }

        if (isWrongPasswordMsg(line)) {
            qCWarning(ARK) << "Wrong password!";
            setPassword(QString());
            Q_EMIT error(i18n("Incorrect password."));
            return false;
        }

        if (isCorruptArchiveMsg(line)) {
            qCWarning(ARK) << "Archive corrupt";
            setCorrupt(true);
            // Special case: corrupt is not a "fatal" error so we return true here.
            return true;
        }

        return readListLine(line);
    }

    if (m_operationMode == Delete) {
        return readDeleteLine(line);
    }

    if (m_operationMode == Test) {

        if (isPasswordPrompt(line)) {
            qCDebug(ARK) << "Found a password prompt";

            Q_EMIT error(i18n("Ark does not currently support testing this archive."));
            return false;
        }

        if (m_cliProps->isTestPassedMsg(line)) {
            qCDebug(ARK) << "Test successful";
            Q_EMIT testSuccess();
            return true;
        }
    }

    return true;
}

bool CliInterface::readDeleteLine(const QString &line)
{
    Q_UNUSED(line);
    return true;
}

bool CliInterface::handleFileExistsMessage(const QString& line)
{
    // Check for a filename and store it.
    if (isFileExistsFileName(line)) {
        const QStringList fileExistsFileNameRegExp = m_cliProps->property("fileExistsFileNameRegExp").toStringList();
        for (const QString &pattern : fileExistsFileNameRegExp) {
            const QRegularExpression rxFileNamePattern(pattern);
            const QRegularExpressionMatch rxMatch = rxFileNamePattern.match(line);

            if (rxMatch.hasMatch()) {
                m_storedFileName = rxMatch.captured(1);
                qCWarning(ARK) << "Detected existing file:" << m_storedFileName;
            }
        }
    }

    if (!isFileExistsMsg(line)) {
        return false;
    }

    Kerfuffle::OverwriteQuery query(QDir::current().path() + QLatin1Char( '/' ) + m_storedFileName);
    query.setNoRenameMode(true);
    query.execute();

    QString responseToProcess;
    const QStringList choices = m_cliProps->property("fileExistsInput").toStringList();

    if (query.responseOverwrite()) {
        responseToProcess = choices.at(0);
    } else if (query.responseSkip()) {
        responseToProcess = choices.at(1);
    } else if (query.responseOverwriteAll()) {
        responseToProcess = choices.at(2);
    } else if (query.responseAutoSkip()) {
        responseToProcess = choices.at(3);
    } else if (query.responseCancelled()) {
        Q_EMIT cancelled();
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

bool CliInterface::doKill()
{
    if (m_process) {
        killProcess(false);
        return true;
    }

    return false;
}

QString CliInterface::escapeFileName(const QString& fileName) const
{
    return fileName;
}

QStringList CliInterface::entryPathDestinationPairs(const QVector<Archive::Entry*> &entriesWithoutChildren, const Archive::Entry *destination)
{
    QStringList pairList;
    if (entriesWithoutChildren.count() > 1) {
        for (const Archive::Entry *file : entriesWithoutChildren) {
            pairList << file->fullPath(NoTrailingSlash) << destination->fullPath() + file->name();
        }
    } else {
        pairList << entriesWithoutChildren.at(0)->fullPath(NoTrailingSlash) << destination->fullPath(NoTrailingSlash);
    }
    return pairList;
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
    m_operationMode = Comment;

    m_commentTempFile.reset(new QTemporaryFile());
    if (!m_commentTempFile->open()) {
        qCWarning(ARK) << "Failed to create temporary file for comment";
        Q_EMIT finished(false);
        return false;
    }

    QTextStream stream(m_commentTempFile.data());
    stream << comment << "\n";
    m_commentTempFile->close();

    if (!runProcess(m_cliProps->property("addProgram").toString(),
                    m_cliProps->commentArgs(filename(), m_commentTempFile->fileName()))) {
        return false;
    }
    m_comment = comment;
    return true;
}

QString CliInterface::multiVolumeName() const
{
    QString oldSuffix = QMimeDatabase().suffixForFileName(filename());
    QString name;

    const QStringList multiVolumeSuffix = m_cliProps->property("multiVolumeSuffix").toStringList();
    for (const QString &multiSuffix : multiVolumeSuffix) {
        QString newSuffix = multiSuffix;
        newSuffix.replace(QStringLiteral("$Suffix"), oldSuffix);
        name = filename().remove(oldSuffix).append(newSuffix);
        if (QFileInfo::exists(name)) {
            break;
        }
    }
    return name;
}

CliProperties *CliInterface::cliProperties() const
{
    return m_cliProps;
}

void CliInterface::onEntry(Archive::Entry *archiveEntry)
{
    if (archiveEntry->compressedSizeIsSet) {
        m_listedSize += archiveEntry->property("compressedSize").toULongLong();
        if (m_listedSize <= m_archiveSizeOnDisk) {
            Q_EMIT progress(float(m_listedSize)/float(m_archiveSizeOnDisk));
        } else {
            // In case summed compressed size exceeds archive size on disk.
            Q_EMIT progress(1);
        }
    }
}

bool CliInterface::isPasswordPrompt(const QString &line)
{
    Q_UNUSED(line);
    return false;
}

bool CliInterface::isWrongPasswordMsg(const QString &line)
{
    Q_UNUSED(line);
    return false;
}

bool CliInterface::isCorruptArchiveMsg(const QString &line)
{
    Q_UNUSED(line);
    return false;
}

bool CliInterface::isDiskFullMsg(const QString &line)
{
    Q_UNUSED(line);
    return false;
}

bool CliInterface::isFileExistsMsg(const QString &line)
{
    Q_UNUSED(line);
    return false;
}

bool CliInterface::isFileExistsFileName(const QString &line)
{
    Q_UNUSED(line);
    return false;
}

}

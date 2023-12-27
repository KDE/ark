/*
    SPDX-FileCopyrightText: 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2009-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/
#ifndef CLIINTERFACE_H
#define CLIINTERFACE_H

#include "archiveentry.h"
#include "archiveinterface.h"
#include "cliproperties.h"
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
class KERFUFFLE_EXPORT CliInterface : public ReadWriteArchiveInterface
{
    Q_OBJECT

public:
    explicit CliInterface(QObject *parent, const QVariantList &args);
    ~CliInterface() override;

    int copyRequiredSignals() const override;

    bool list() override;
    bool extractFiles(const QVector<Archive::Entry *> &files, const QString &destinationDirectory, const ExtractionOptions &options) override;
    bool addFiles(const QVector<Archive::Entry *> &files,
                  const Archive::Entry *destination,
                  const CompressionOptions &options,
                  uint numberOfEntriesToAdd = 0) override;
    bool moveFiles(const QVector<Archive::Entry *> &files, Archive::Entry *destination, const CompressionOptions &options) override;
    bool copyFiles(const QVector<Archive::Entry *> &files, Archive::Entry *destination, const CompressionOptions &options) override;
    bool deleteFiles(const QVector<Archive::Entry *> &files) override;
    bool addComment(const QString &comment) override;
    bool testArchive() override;

    virtual void resetParsing() = 0;
    virtual bool readListLine(const QString &line) = 0;
    virtual bool readExtractLine(const QString &line) = 0;
    virtual bool readDeleteLine(const QString &line);
    virtual bool isPasswordPrompt(const QString &line);
    virtual bool isWrongPasswordMsg(const QString &line);
    virtual bool isCorruptArchiveMsg(const QString &line);
    virtual bool isDiskFullMsg(const QString &line);
    virtual bool isFileExistsMsg(const QString &line);
    virtual bool isFileExistsFileName(const QString &line);
    virtual bool isNewMovedFileNamesMsg(const QString &line);
    bool doKill() override;

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

    /**
     * @see ArchiveModel::entryPathsFromDestination
     */
    void setNewMovedFiles(const QVector<Archive::Entry *> &entries, const Archive::Entry *destination, int entriesWithoutChildren);

    /**
     * @return The list of selected files to extract.
     */
    QStringList extractFilesList(const QVector<Archive::Entry *> &files) const;

    QString multiVolumeName() const override;

    CliProperties *cliProperties() const;

protected:
    bool setAddedFiles();

    /**
     * Handles the given @p line.
     * @return True if the line is ok. False if the line contains/triggers a "fatal" error
     * or a canceled user query. If false is returned, the caller is supposed to call killProcess().
     */
    virtual bool handleLine(const QString &line);

    /**
     * Run @p programName with the given @p arguments.
     *
     * @param programName The program that will be run (not the whole path).
     * @param arguments A list of arguments that will be passed to the program.
     *
     * @return @c true if the program was found and the process was started correctly,
     *         @c false otherwise (in which case finished(false) is emitted).
     */
    bool runProcess(const QString &programName, const QStringList &arguments);

    /**
     * Kill the running process. The finished signal is emitted according to @p emitFinished.
     */
    void killProcess(bool emitFinished = true);

    /**
     * Ask the password *before* running any process.
     * @return True if the user supplies a password, false otherwise (in which case finished() is emitted).
     */
    bool passwordQuery();

    void cleanUp();

    CliProperties *m_cliProps = nullptr;
    QString m_oldWorkingDirExtraction; // Used ONLY by extraction jobs.
    QString m_oldWorkingDir; // Used by copy and move jobs.
    QScopedPointer<QTemporaryDir> m_tempWorkingDir;
    QScopedPointer<QTemporaryDir> m_tempAddDir;
    OperationMode m_subOperation = NoOperation;
    QVector<Archive::Entry *> m_passedFiles;
    QVector<Archive::Entry *> m_tempAddedFiles;
    Archive::Entry *m_passedDestination = nullptr;
    CompressionOptions m_passedOptions;

#ifdef Q_OS_WIN
    KProcess *m_process = nullptr;
#else
    KPtyProcess *m_process = nullptr;
#endif

    bool m_abortingOperation = false;

protected Q_SLOTS:
    virtual void readStdout(bool handleAll = false);

private:
    bool handleFileExistsMessage(const QString &filename);

    /**
     * Returns a list of path pairs which will be supplied to rn command.
     * <src_file_1> <dest_file_1> [ <src_file_2> <dest_file_2> ... ]
     * Also constructs a list of new entries resulted in moving.
     *
     * @param entriesWithoutChildren List of archive entries
     * @param destination Must be a directory entry if QList contains more that one entry
     */
    QStringList entryPathDestinationPairs(const QVector<Archive::Entry *> &entriesWithoutChildren, const Archive::Entry *destination);

    /**
     * Wrapper around KProcess::write() or KPtyDevice::write(), depending on
     * the platform.
     */
    void writeToProcess(const QByteArray &data);

    /**
     * Moves the dropped @files from the temp dir to the @p finalDest.
     * @return @c true if the files have been moved, @c false otherwise.
     */
    bool moveDroppedFilesToDest(const QVector<Archive::Entry *> &files, const QString &finalDest);

    /**
     * @return Whether @p dir is an empty directory.
     */
    bool isEmptyDir(const QDir &dir);

    /**
     * Performs any additional escaping and processing on @p fileName
     * before passing it to the underlying process.
     *
     * The default implementation returns @p fileName unchanged.
     *
     * @param fileName String to escape.
     */
    virtual QString escapeFileName(const QString &fileName) const;

    void cleanUpExtracting();
    void restoreWorkingDirExtraction();

    void finishCopying(bool result);

    QByteArray m_stdOutData;
    QRegularExpression m_passwordPromptPattern;
    QHash<int, QList<QRegularExpression>> m_patternCache;

    QVector<Archive::Entry *> m_removedFiles;
    QVector<Archive::Entry *> m_newMovedFiles;
    int m_exitCode = 0;
    bool m_listEmptyLines = false;
    QString m_storedFileName;

    ExtractionOptions m_extractionOptions;
    QString m_extractDestDir;
    QScopedPointer<QTemporaryDir> m_extractTempDir;
    QScopedPointer<QTemporaryFile> m_commentTempFile;
    QVector<Archive::Entry *> m_extractedFiles;
    qulonglong m_archiveSizeOnDisk = 0;
    qulonglong m_listedSize = 0;

    QMetaObject::Connection m_readyStdOutConnection;

protected Q_SLOTS:
    virtual void processFinished(int exitCode, QProcess::ExitStatus exitStatus);

private Q_SLOTS:
    void extractProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
    void continueCopying(bool result);
    void onEntry(Archive::Entry *archiveEntry);
};
}

#endif /* CLIINTERFACE_H */

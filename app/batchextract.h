/*
    SPDX-FileCopyrightText: 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2009-2010 Raphael Kubo da Costa <rakuco@FreeBSD.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef BATCHEXTRACT_H
#define BATCHEXTRACT_H

#include <KCompositeJob>

#include <QList>
#include <QMap>

namespace Kerfuffle
{
class Archive;
class Query;
}

/**
 * This class schedules the extraction of all given compressed archives.
 *
 * Like AddToArchive, this class does not need the GUI to be active, and
 * provides the functionality available from the --batch command-line option.
 *
 * @author Harald Hvaal <haraldhv@stud.ntnu.no>
 */
class BatchExtract : public KCompositeJob
{
    Q_OBJECT

public:
    /**
     * Creates a new BatchExtract object.
     */
    explicit BatchExtract(QObject *parent = nullptr);

    /**
     * Destroys a BatchExtract object.
     */
    ~BatchExtract() override;

    /**
     * Creates a BatchExtractJob for the given @p url and puts it on the queue.
     *
     * If necessary, the destination directory for the archive is created by the job.
     *
     * @param url The url of the archive that will be extracted.
     *
     * @see setAutoSubfolder
     */
    void addExtraction(const QUrl &url);

    bool doKill() override;

    /**
     * A wrapper that calls slotStartJob() when the event loop has started.
     */
    void start() override;

    /**
     * Whether to automatically create a folder inside the destination
     * directory if the archive has more than one directory or file
     * at top level.
     *
     * @return @c true  Create the subdirectory automatically.
     * @return @c false Do not create the subdirectory automatically.
     */
    bool autoSubfolder() const;

    /**
     * Set whether a folder should be created when necessary so
     * the archive is extracted to it.
     *
     * If set to @c true, when the archive does not consist of a
     * single folder with the other files and directories inside,
     * a directory will be automatically created inside the destination
     * directory and the archive will be extracted there.
     *
     * @param value Whether to create this directory automatically
     *              when needed.
     */
    void setAutoSubfolder(bool value);

    /**
     * Adds a file to the list of files that will be extracted.
     *
     * @param url The file that will be added to the list.
     */
    void addInput(const QUrl &url);

    /**
     * Shows the extract options dialog before extracting the files.
     *
     * @return @c true  The user has set some options and clicked OK.
     * @return @c false The user has canceled extraction.
     */
    bool showExtractDialog();

    /**
     * Returns the destination directory where the archives
     * will be extracted to.
     *
     * @return The destination directory. If no directory has been manually
     *         set with setDestinationFolder, QDir::currentPath() will be
     *         returned.
     */
    QString destinationFolder() const;

    /**
     * Sets the directory the archives will be extracted to.
     *
     * If @c setSubfolder has been used, the final destination
     * directory will be the concatenation of both.
     *
     * If @p folder does not exist, the current destination
     * folder will not change.
     *
     * @param folder The directory that will be used.
     */
    void setDestinationFolder(const QString &folder);

    /**
     * Returns whether the destination folder should
     * be open after all archives are extracted.
     *
     * @return @c true  Open the destination folder.
     * @return @c false Do not open the destination folder.
     */
    bool openDestinationAfterExtraction() const;

    /**
     * Whether to open the destination folder after
     * all archives are extracted.
     *
     * @param value Whether to open the destination.
     */
    void setOpenDestinationAfterExtraction(bool value);

    /**
     * Whether all files should be extracted to the same directory,
     * even if they're in different directories in the archive.
     *
     * This is also known as "flat" extraction.
     *
     * @return @c true  Paths should be preserved.
     * @return @c false Paths should be ignored.
     */
    bool preservePaths() const;

    /**
     * Sets whether paths should be preserved during extraction.
     *
     * When it is set to false, all files are extracted to a single
     * directory, regardless of their hierarchy in the archive.
     *
     * @param value Whether to preserve paths.
     */
    void setPreservePaths(bool value);

private Q_SLOTS:
    /**
     * Updates the percentage of the job that has been completed.
     */
    void forwardProgress(KJob *job, unsigned long percent);

    /**
     * Shows a dialog with a list of all the files that could not
     * be successfully extracted.
     */
    void showFailedFiles();

    /**
     * Shows an error message if the current job hasn't finished
     * successfully, and advances to the next extraction job if
     * there are more.
     */
    void slotResult(KJob *job) override;

    /**
     * Shows a query dialog, which may happen when a file already exists.
     */
    void slotUserQuery(Kerfuffle::Query *query);

    /**
     * Does the real work for start() and extracts all scheduled files.
     *
     * Each extraction job is started after the last one finishes.
     * The jobs are executed in the order they were added via addInput().
     */
    void slotStartJob();

private:
    int m_initialJobCount;
    QMap<KJob *, QPair<QString, QString>> m_fileNames;
    bool m_autoSubfolder;

    QList<QUrl> m_inputs;
    QString m_destinationFolder;
    QStringList m_failedFiles;
    bool m_preservePaths;
    bool m_openDestinationAfterExtraction;
};

#endif // BATCHEXTRACT_H

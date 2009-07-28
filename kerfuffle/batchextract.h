/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv (at@at) stud.ntnu.no>
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

#ifndef BATCHEXTRACT_H
#define BATCHEXTRACT_H

#include "kerfuffle_export.h"

#include <kcompositejob.h>
#include <KUrl>

#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Kerfuffle
{
class Archive;
class Query;

/**
 * This class schedules the extraction of all given compressed archives.
 *
 * Like AddToArchive, this class does not need the GUI to be active, and
 * provides the functionality avaliable from the --batch command-line option.
 *
 * @author Harald Hvaal <haraldhv@stud.ntnu.no>
 */
class KERFUFFLE_EXPORT BatchExtract : public KCompositeJob
{
    Q_OBJECT

public:
    /**
     * Creates a new BatchExtract object.
     */
    BatchExtract();

    /**
     * Destroys a BatchExtract object.
     */
    virtual ~BatchExtract();

    /**
     * Creates an ExtractJob for the given @p archive and puts it on the queue.
     * 
     * If necessary, the destination directory for the archive is created.
     *
     * @param archive           The archive that will be extracted.
     *
     * @see setAutoSubfolder
     */
    void addExtraction(Archive* archive);

    /**
     * Starts the extraction of all files.
     *
     * Each extraction job is started after the last one finishes.
     * The jobs are executed in the order they were added via addInput.
     */
    void start();

    /**
     * Whether to automatically create a folder inside the destination
     * directory if the archive has more than one directory or file
     * at top level.
     *
     * @return @c true  Create the subdirectory automatically.
     * @return @c false Do not create the subdirectory automatically.
     */
    bool autoSubfolder();

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
     *
     * @return @c true  The file exists and a suitable plugin
     *                  could be found for it.
     * @return @c false The file does not exist or a suitable
     *                  plugin could not be found.
     */
    bool addInput(const KUrl& url);

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
    QString destinationFolder();

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
    void setDestinationFolder(QString folder);

    /**
     * Returns the subdirectory into which the archives
     * will be extracted, if it has been set.
     *
     * @return The subdirectory that will be used, or an
     *         empty @c QString if none has been set.
     */
    QString subfolder();

    /**
     * Forces the creation of a subdirectory inside the destination
     * directory, so that the archives are extracted into it.
     *
     * @param subfolder The subdirectory that will be created.
     */
    void setSubfolder(QString subfolder);

    /**
     * Whether all files should be extracted to the same directory,
     * even if they're in different directories in the archive.
     *
     * This is also known as "flat" extraction.
     *
     * @return @c true  Paths should be preserved.
     * @return @c false Paths should be ignored.
     */
    bool preservePaths();

    /**
     * Sets whether paths should be preserved during extraction.
     *
     * When it is set to false, all files are extracted to a single
     * directory, regardless of their hierarchy in the archive.
     *
     * @param value Whether to preserve paths.
     */
    void setPreservePaths(bool value);

private slots:
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
    void slotResult(KJob *job);

    /**
     * Shows a query dialog, which may happen when a file already exists.
     */
    void slotUserQuery(Query *query);

private:
    int m_initialJobCount;
    QMap<KJob*, QPair<QString, QString> > m_fileNames;
    bool m_autoSubfolder;

    QList<Archive*> m_inputs;
    QString m_destinationFolder;
    QStringList m_failedFiles;
    QString m_subfolder;
    bool m_preservePaths;
};
}

#endif // BATCHEXTRACT_H

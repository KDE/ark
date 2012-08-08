/* This file is part of the KDE libraries
    Copyright (C) 2000 Stephan Kulow <coolo@kde.org>
                  1999 - 2008 David Faure <faure@kde.org>
                  2001 Holger Freyther <freyther@kde.org>
    Copyright (C) 2012 basysKom GmbH <info@basyskom.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef KERFUFFLE_RENAMEDIALOG_H
#define KERFUFFLE_RENAMEDIALOG_H

#include <sys/types.h>
#include <QtGui/QDialog>
#include <QtCore/QString>
#include <kio/global.h>
#include <KSqueezedTextLabel>
#include <KUrl>
#include "../kerfuffle_export.h"

class QScrollArea;
class QLabel;
class QPixmap;
class KFileItem;

namespace Kerfuffle
{

// KDE5: get rid of M_OVERWRITE_ITSELF, trigger it internally if src==dest
// KDE5: get rid of M_SINGLE. If not multi, then single ;)
// KDE5: use QFlags to get rid of all the casting!
/**
 * M_OVERWRITE: We have an existing dest, show details about it and offer to overwrite it.
 * M_OVERWRITE_ITSELF: Warn that the current operation would overwrite a file with itself,
 *                     which is not allowed.
 * M_SKIP: Offer a "Skip" button, to skip other files too. Requires M_MULTI.
 * M_SINGLE: Deprecated and unused, please ignore.
 * M_MULTI: Set if the current operation concerns multiple files, so it makes sense
 *  to offer buttons that apply the user's choice to all files/folders.
 * M_RESUME: Offer a "Resume" button (plus "Resume All" if M_MULTI)
 * M_NORENAME: Don't offer a "Rename" button
 * M_ISDIR: The dest is a directory, so label the "overwrite" button something like "merge" instead.
 * M_UPDATE_EXISTING: Offer a "Only overwrite existing files" button. Also sets M_MULTI
 * M_AUTO_RENAME: Offer a "Rename" button but no new name entry, show a label with
 * automagically created name instead. Mutually exclusive with by M_NORENAME
 */
enum RenameDialog_Mode { M_OVERWRITE = 1, M_OVERWRITE_ITSELF = 2, M_SKIP = 4, M_SINGLE = 8, M_MULTI = 16, M_RESUME = 32, M_NORENAME = 64, M_ISDIR = 128, M_UPDATE_EXISTING = 256, M_AUTO_RENAME = 512};

/**
 * The result of open_RenameDialog().
 */
enum RenameDialog_Result {R_RESUME = 6, R_RESUME_ALL = 7, R_OVERWRITE = 4, R_OVERWRITE_ALL = 5, R_SKIP = 2, R_AUTO_SKIP = 3, R_RENAME = 1, R_AUTO_RENAME = 8, R_CANCEL = 0, R_UPDATE_EXISTING};


/**
 * The dialog shown when a CopyJob realizes that a destination file already exists,
 * and wants to offer the user with the choice to either Rename, Overwrite, Skip;
 * this dialog is also used when a .part file exists and the user can choose to
 * Resume a previous download.
 */
class KERFUFFLE_EXPORT RenameDialog : public QDialog
{
    Q_OBJECT
public:
    /**
     * Construct a "rename" dialog to let the user know that @p src is about to overwrite @p dest.
     *
     * @param parent parent widget (often 0)
     * @param caption the caption for the dialog box
     * @param src the url to the file/dir we're trying to copy, as it's part of the text message
     * @param dest the path to destination file/dir, i.e. the one that already exists
     * @param mode parameters for the dialog (which buttons to show...),
     * @param sizeSrc size of source file
     * @param sizeDest size of destination file
     * @param ctimeSrc creation time of source file
     * @param ctimeDest creation time of destination file
     * @param mtimeSrc modification time of source file
     * @param mtimeDest modification time of destination file
     * @see RenameDialog_Mode
     */
    RenameDialog(QWidget *parent, const QString & caption,
                 const KUrl & src, const KUrl & dest,
                 RenameDialog_Mode mode,
                 KIO::filesize_t sizeSrc = KIO::filesize_t(-1),
                 KIO::filesize_t sizeDest = KIO::filesize_t(-1),
                 time_t ctimeSrc = time_t(-1),
                 time_t ctimeDest = time_t(-1),
                 time_t mtimeSrc = time_t(-1),
                 time_t mtimeDest = time_t(-1));
    ~RenameDialog();

    /**
     * @return the new destination
     * valid only if RENAME was chosen
     */
    KUrl newDestUrl();


    /**
     * @return an automatically renamed destination
     * @since 4.5
     * valid always
     */
    KUrl autoDestUrl() const;

    /**
     * Given a directory path and a filename (which usually exists already),
     * this function returns a suggested name for a file that doesn't exist
     * in that directory. The existence is only checked for local urls though.
     * The suggested file name is of the form "foo_1", "foo_2" etc.
     */
    static QString suggestName(const KUrl& baseURL, const QString& oldName);

public Q_SLOTS:
    void cancelPressed();
    void renamePressed();
    void skipPressed();
    void autoSkipPressed();
    void overwritePressed();
    void overwriteAllPressed();
    void resumePressed();
    void resumeAllPressed();
    void updateExistingPressed();
    void suggestNewNamePressed();

protected Q_SLOTS:
    void enableRenameButton(const QString &);
private Q_SLOTS:
    void applyAllPressed();
    void showSrcIcon(const KFileItem &);
    void showDestIcon(const KFileItem &);
    void showSrcPreview(const KFileItem &, const QPixmap &);
    void showDestPreview(const KFileItem &, const QPixmap &);
    void resizePanels();

private:
    QScrollArea* createContainerLayout(QWidget* parent, const KFileItem& item, QLabel* preview);
    QLabel* createLabel(QWidget* parent, const QString& text, const bool containerTitle);
    KSqueezedTextLabel* createSqueezedLabel(QWidget* parent, const QString& text);
    class RenameDialogPrivate;
    RenameDialogPrivate* const d;
};

}

#endif

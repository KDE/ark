/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (C) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
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
#ifndef EXTRACTIONDIALOG_H
#define EXTRACTIONDIALOG_H

#include "kerfuffle_export.h"

#include <KDirSelectDialog>

#include <KDialog>

namespace Kerfuffle
{
class KERFUFFLE_EXPORT ExtractionDialog: public KDirSelectDialog
{
    Q_OBJECT
public:
    ExtractionDialog(QWidget *parent = 0);
    ~ExtractionDialog();

    void setShowSelectedFiles(bool);
    void setSingleFolderArchive(bool);
    void setPreservePaths(bool);
    void batchModeOption();
    void setOpenDestinationFolderAfterExtraction(bool);
    void setAutoSubfolder(bool value);

    bool extractAllFiles();
    bool openDestinationAfterExtraction();
    bool extractToSubfolder();
    bool autoSubfolders();
    bool preservePaths();
    KUrl destinationDirectory();
    QString subfolder() const;
    virtual void accept();

public Q_SLOTS:
    void setCurrentUrl(const QString& url);
    void setSubfolder(QString subfolder);

private Q_SLOTS:
    void writeSettings();

private:
    void loadSettings();

    class ExtractionDialogUI *m_ui;
};
}

#endif // EXTRACTIONDIALOG_H

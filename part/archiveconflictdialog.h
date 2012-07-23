/*
 * archiveconflictdialog.h
 *
 * Copyright (C) 2012 basysKom GmbH <info@basyskom.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef CONFLICTDIALOG_H
#define CONFLICTDIALOG_H

#include <KDialog>

class QString;
class QRadioButton;

class ArchiveConflictDialog : public KDialog
{
    Q_OBJECT

public:
    ArchiveConflictDialog(QWidget * parent, QString archive, QString suggestedName);

    enum ConflictOption {
        OverwriteExisting = 1,
        RenameNew = 2,
        OpenExisting = 3
    };

    int selectedOption();

private:
    QRadioButton *m_overwriteButton;
    QRadioButton *m_saveAsButton;
    QRadioButton *m_openExistingButton;
};

#endif

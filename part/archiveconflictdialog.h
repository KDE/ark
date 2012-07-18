/*
 * Copyright (C) 2012 basysKom GmbH <info@basyskom.com>
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

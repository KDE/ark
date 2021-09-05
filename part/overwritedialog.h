/*
    ark -- archiver for the KDE project

    SPDX-FileCopyrightText: 2016 Ragnar Thomsen <rthomsen6@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef OVERWRITEDIALOG_H
#define OVERWRITEDIALOG_H

#include "archiveentry.h"


#include <QDialog>
#include <QLabel>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>


class OverwriteDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OverwriteDialog(QWidget *parent, const QList<const Kerfuffle::Archive::Entry*> &entries, bool error = false);
    ~OverwriteDialog() override;

private:
    QVBoxLayout m_vBoxLayout;
    QHBoxLayout m_messageLayout;
    QLabel m_messageIcon;
    QLabel m_messageText;
    QListWidget m_entriesList;
    QDialogButtonBox m_buttonBox;
};

#endif

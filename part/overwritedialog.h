/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2016 Ragnar Thomsen <rthomsen6@gmail.com>
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

#ifndef OVERWRITEDIALOG_H
#define OVERWRITEDIALOG_H

#include "kerfuffle_export.h"
#include "kerfuffle/archiveentry.h"

#include <KFileWidget>
#include <KLocalizedString>

#include <QDialog>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QDialogButtonBox>

class QUrl;

namespace Kerfuffle
{
class KERFUFFLE_EXPORT OverwriteDialog : public QDialog
{
    Q_OBJECT
public:
    explicit OverwriteDialog(QWidget *parent, const QList<const Archive::Entry*> &entries, const QHash<QString, QPixmap> &icons, bool error = false);
    virtual ~OverwriteDialog();

private:
    QVBoxLayout m_vBoxLayout;
    QHBoxLayout m_messageLayout;
    QLabel m_messageIcon;
    QLabel m_messageText;
    QListWidget m_entriesList;
    QDialogButtonBox m_buttonBox;
    QPushButton m_okButton;
    QPushButton m_cancelButton;
};
}

#endif

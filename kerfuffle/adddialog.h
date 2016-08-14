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

#ifndef ADDDIALOG_H
#define ADDDIALOG_H

#include "kerfuffle_export.h"
#include "archive_kerfuffle.h"
#include "compressionoptionswidget.h"

#include <KFileWidget>

#include <QDialog>
#include <QMimeType>

class QUrl;

namespace Kerfuffle
{
class KERFUFFLE_EXPORT AddDialog : public QDialog
{
    Q_OBJECT
public:
    explicit AddDialog(QWidget *parent,
                       const QString &title,
                       const QUrl &startDir,
                       const QMimeType &mimeType,
                       const CompressionOptions &opts = QHash<QString, QVariant>());
    virtual ~AddDialog();
    QStringList selectedFiles() const;
    CompressionOptions compressionOptions() const;
    QDialog *optionsDialog;

private:
    KFileWidget *m_fileWidget;
    QMimeType m_mimeType;
    CompressionOptions m_compOptions;

public slots:
    void slotOpenOptions();
};
}

#endif

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

#ifndef COMPRESSIONOPTIONSWIDGET_H
#define COMPRESSIONOPTIONSWIDGET_H

#include "kerfuffle_export.h"
#include "archive_kerfuffle.h"
#include "archiveformat.h"
#include "ui_compressionoptionswidget.h"

#include <QMimeType>
#include <QWidget>

namespace Kerfuffle
{
class KERFUFFLE_EXPORT CompressionOptionsWidget : public QWidget, public Ui::CompressionOptionsWidget
{
    Q_OBJECT

public:
    explicit CompressionOptionsWidget(QWidget *parent = nullptr,
                                      const CompressionOptions &opts = {});
    int compressionLevel() const;
    QString compressionMethod() const;
    QString encryptionMethod() const;
    ulong volumeSize() const;
    QString password() const;
    CompressionOptions commpressionOptions() const;
    bool isEncryptionAvailable() const;
    bool isEncryptionEnabled() const;
    bool isHeaderEncryptionAvailable() const;
    bool isHeaderEncryptionEnabled() const;
    KNewPasswordWidget::PasswordStatus passwordStatus() const;

    void setEncryptionVisible(bool visible);
    void setMimeType(const QMimeType &mimeType);

private:
    void updateWidgets();
    ArchiveFormat archiveFormat() const;

    QMimeType m_mimetype;
    CompressionOptions m_opts;

private Q_SLOTS:
    void slotMultiVolumeChecked(int state);
    void slotCompMethodChanged(const QString &value);
    void slotEncryptionMethodChanged(const QString &value);
};
}

#endif

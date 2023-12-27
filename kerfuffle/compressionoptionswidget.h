/*
    SPDX-FileCopyrightText: 2016 Ragnar Thomsen <rthomsen6@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef COMPRESSIONOPTIONSWIDGET_H
#define COMPRESSIONOPTIONSWIDGET_H

#include "archive_kerfuffle.h"
#include "archiveformat.h"
#include "kerfuffle_export.h"
#include "ui_compressionoptionswidget.h"

#include <QMimeType>
#include <QWidget>

namespace Kerfuffle
{
class KERFUFFLE_EXPORT CompressionOptionsWidget : public QWidget, public Ui::CompressionOptionsWidget
{
    Q_OBJECT

public:
    explicit CompressionOptionsWidget(QWidget *parent = nullptr, const CompressionOptions &opts = {});
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

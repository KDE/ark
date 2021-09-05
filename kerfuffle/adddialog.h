/*
    ark -- archiver for the KDE project

    SPDX-FileCopyrightText: 2016 Ragnar Thomsen <rthomsen6@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
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
                       const CompressionOptions &opts = {});
    ~AddDialog() override;
    QStringList selectedFiles() const;
    CompressionOptions compressionOptions() const;
    QDialog *optionsDialog;

private:
    KFileWidget *m_fileWidget;
    QMimeType m_mimeType;
    CompressionOptions m_compOptions;

public Q_SLOTS:
    void slotOpenOptions();
};
}

#endif

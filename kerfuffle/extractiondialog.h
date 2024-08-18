/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008 Harald Hvaal <haraldhv@stud.ntnu.no>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef EXTRACTIONDIALOG_H
#define EXTRACTIONDIALOG_H

#include "kerfuffle_export.h"

#include <QDialog>
#include <QUrl>

#include <KFileWidget>

namespace Kerfuffle
{
class KERFUFFLE_EXPORT ExtractionDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ExtractionDialog(QWidget *parent = nullptr);
    ~ExtractionDialog() override;

    void setShowSelectedFiles(bool);
    void setExtractToSubfolder(bool);
    void setPreservePaths(bool);
    void batchModeOption();
    void setOpenDestinationFolderAfterExtraction(bool);
    void setCloseAfterExtraction(bool);
    void setAutoSubfolder(bool value);

    bool extractAllFiles() const;
    bool openDestinationAfterExtraction() const;
    bool closeAfterExtraction() const;
    bool extractToSubfolder() const;
    bool autoSubfolders() const;
    bool preservePaths() const;
    QUrl destinationDirectory() const;
    QString subfolder() const;

public Q_SLOTS:
    void setBusyGui();
    void setReadyGui();
    void setSubfolder(const QString &subfolder);
    void setCurrentUrl(const QUrl &url);
    void restoreWindowSize();

private Q_SLOTS:
    void writeSettings();
    void slotAccepted();

private:
    void loadSettings();

    class ExtractionDialogUI *m_ui;
    KFileWidget *fileWidget = nullptr;
};
}

#endif // EXTRACTIONDIALOG_H

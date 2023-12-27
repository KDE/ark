/*
    SPDX-FileCopyrightText: 2016 Ragnar Thomsen <rthomsen6@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef PROPERTIESDIALOG_H
#define PROPERTIESDIALOG_H

#include "kerfuffle_export.h"

#include <QCryptographicHash>
#include <QDialog>

class QLabel;

namespace Kerfuffle
{
class Archive;

class KERFUFFLE_EXPORT PropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PropertiesDialog(QWidget *parent, Archive *archive, qulonglong numberOfFiles, qulonglong numberOfFolders, qulonglong size);

private:
    QString calcHash(QCryptographicHash::Algorithm algorithm, const QString &path);
    void showChecksum(QCryptographicHash::Algorithm algorithm, const QString &fileName, QLabel *label);

    class PropertiesDialogUI *m_ui;
};
}

#endif

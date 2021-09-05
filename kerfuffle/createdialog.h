/*
    ark -- archiver for the KDE project

    SPDX-FileCopyrightText: 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2015 Elvis Angelaccio <elvis.angelaccio@kde.org>
    SPDX-FileCopyrightText: 2016 Ragnar Thomsen <rthomsen6@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef CREATEDIALOG_H
#define CREATEDIALOG_H

#include "archive_kerfuffle.h"
#include "kerfuffle_export.h"
#include "pluginmanager.h"

#include <KConfigGroup>

#include <QDialog>
#include <QMimeType>

class QUrl;
class QVBoxLayout;

namespace Kerfuffle
{

class KERFUFFLE_EXPORT CreateDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateDialog(QWidget *parent,
                          const QString &caption,
                          const QUrl &startDir);
    void setFileName(const QString &fileName);
    QUrl selectedUrl() const;
    QString password() const;
    QMimeType currentMimeType() const;
    bool setMimeType(const QString &mimeTypeName);
    int compressionLevel() const;
    QString compressionMethod() const;
    QString encryptionMethod() const;
    ulong volumeSize() const;

    /**
     * @return Whether the user can encrypt the new archive.
     */
    bool isEncryptionAvailable() const;

    /**
     * @return Whether the user has chosen to encrypt the new archive.
     */
    bool isEncryptionEnabled() const;

    /**
     * @return Whether the user can encrypt the list of files in the new archive.
     */
    bool isHeaderEncryptionAvailable() const;

    /**
     * @return Whether the user has chosen to encrypt the list of files in the new archive.
     */
    bool isHeaderEncryptionEnabled() const;

public Q_SLOTS:
    void accept() override;

private:
    void loadConfiguration();

    class CreateDialogUI *m_ui;
    QVBoxLayout *m_vlayout;
    KConfigGroup m_config;
    QStringList m_supportedMimeTypes;
    PluginManager m_pluginManger;

private Q_SLOTS:
    void slotFileNameEdited(const QString &text);
    void slotUpdateWidgets(int index);
    void slotUpdateDefaultMimeType();
    void slotUpdateFilenameExtension(int index);
};
}

#endif

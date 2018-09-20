/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (C) 2015 Elvis Angelaccio <elvis.angelaccio@kde.org>
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

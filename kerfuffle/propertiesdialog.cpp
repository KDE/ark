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

#include "propertiesdialog.h"
#include "ark_debug.h"
#include "ui_propertiesdialog.h"

#include <QDateTime>
#include <QFileInfo>

#include <KIconLoader>
#include <KIO/Global>

namespace Kerfuffle
{
class PropertiesDialogUI: public QWidget, public Ui::PropertiesDialog
{
public:
    PropertiesDialogUI(QWidget *parent = 0)
            : QWidget(parent) {
        setupUi(this);
    }
};

PropertiesDialog::PropertiesDialog(QWidget *parent, Archive *archive)
        : QDialog(parent, Qt::Dialog)
{
    qCDebug(ARK) << "PropertiesDialog loaded";

    QFileInfo fi(archive->fileName());

    setWindowTitle(i18nc("@title:window", "Properties for %1", fi.fileName()));
    setModal(true);

    m_ui = new PropertiesDialogUI(this);
    m_ui->lblArchiveName->setText(archive->fileName());
    m_ui->lblArchiveType->setText(archive->mimeType().comment());
    m_ui->lblMimetype->setText(archive->mimeType().name());
    m_ui->lblReadOnly->setText(archive->isReadOnly() ?  i18n("yes") : i18n("no"));
    m_ui->lblPasswordProtected->setText(archive->isPasswordProtected() ?  i18n("yes") : i18n("no"));
    m_ui->lblHasComment->setText(archive->hasComment() ?  i18n("yes") : i18n("no"));
    m_ui->lblNumberOfFiles->setText(QString::number(archive->numberOfFiles()));
    m_ui->lblUnpackedSize->setText(KIO::convertSize(archive->unpackedSize()));
    m_ui->lblPackedSize->setText(KIO::convertSize(archive->packedSize()));
    m_ui->lblCompressionRatio->setText(QString::number(float(archive->unpackedSize()) / float(archive->packedSize()), 'f', 1));
    m_ui->lblLastModified->setText(fi.lastModified().toString(QStringLiteral("yyyy-MM-dd HH:mm")));

    // Show an icon representing the mimetype of the archive.
    QIcon icon = QIcon::fromTheme(archive->mimeType().iconName());
    m_ui->lblIcon->setPixmap(icon.pixmap(IconSize(KIconLoader::Desktop), IconSize(KIconLoader::Desktop)));

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);

    m_ui->adjustSize();
    setFixedSize(m_ui->size());
}


}

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
#include "archive_kerfuffle.h"
#include "ark_debug.h"
#include "ui_propertiesdialog.h"

#include <QtConcurrentRun>
#include <QDateTime>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFutureWatcher>
#include <QIcon>

#include <KIO/Global>

namespace Kerfuffle
{
class PropertiesDialogUI: public QWidget, public Ui::PropertiesDialog
{
    Q_OBJECT

public:
    PropertiesDialogUI(QWidget *parent = nullptr)
            : QWidget(parent) {
        setupUi(this);
    }
};

PropertiesDialog::PropertiesDialog(QWidget *parent, Archive *archive, qulonglong numberOfFiles, qulonglong numberOfFolders, qulonglong size)
        : QDialog(parent, Qt::Dialog)
{
    setAttribute(Qt::WA_DeleteOnClose, true);
    QFileInfo fi(archive->fileName());

    setWindowTitle(i18nc("@title:window", "Properties for %1", fi.fileName()));
    setModal(true);

    m_ui = new PropertiesDialogUI(this);
    m_ui->lblArchiveName->setText(archive->fileName());
    m_ui->lblArchiveType->setText(archive->mimeType().comment());
    m_ui->lblMimetype->setText(archive->mimeType().name());
    m_ui->lblCompressionMethods->setText(archive->property("compressionMethods").toStringList().join(QLatin1String(", ")));
    m_ui->lblReadOnly->setText(archive->isReadOnly() ?  i18n("yes") : i18n("no"));
    m_ui->lblMultiVolume->setText(archive->isMultiVolume() ? i18n("yes (%1 volumes)", archive->numberOfVolumes()) : i18n("no"));
    m_ui->lblHasComment->setText(archive->hasComment() ?  i18n("yes") : i18n("no"));
    m_ui->lblNumberOfEntries->setText(i18np("%1 file", "%1 files", numberOfFiles) +
                                      i18np(", %1 folder", ", %1 folders", numberOfFolders));
    m_ui->lblUnpackedSize->setText(KIO::convertSize(size));
    m_ui->lblPackedSize->setText(KIO::convertSize(archive->packedSize()));
    m_ui->lblCompressionRatio->setText(QString::number(float(archive->unpackedSize()) / float(archive->packedSize()), 'f', 1));
    m_ui->lblLastModified->setText(fi.lastModified().toString(QStringLiteral("yyyy-MM-dd HH:mm")));
    m_ui->lblMD5->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_ui->lblSHA1->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_ui->lblSHA256->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    switch (archive->encryptionType()) {
    case Archive::Unencrypted:
        m_ui->lblPasswordProtected->setText(i18n("no"));
        break;
    case Archive::Encrypted:
        m_ui->lblPasswordProtected->setText(i18n("only file contents (%1)", archive->property("encryptionMethods").toStringList().join(QLatin1String(", "))));
        break;
    case Archive::HeaderEncrypted:
        m_ui->lblPasswordProtected->setText(i18n("yes (%1)", archive->property("encryptionMethods").toStringList().join(QLatin1String(", "))));
        break;
    }

    // The Sha256 label is populated with 64 chars in the ui file. We fix the
    // size of the label so the dialog won't resize when the hashes are
    // calculated. This is an ugly hack and requires e.g. that we use monospace
    // font for the hashes.
    m_ui->lblSHA256->setMinimumSize(m_ui->lblSHA256->sizeHint());

    m_ui->adjustSize();
    setFixedSize(m_ui->size());

    // Show an icon representing the mimetype of the archive.
    QIcon icon = QIcon::fromTheme(archive->mimeType().iconName());
    m_ui->lblIcon->setPixmap(icon.pixmap(48));

    m_ui->lblMD5->setText(i18n("Calculating..."));
    m_ui->lblSHA1->setText(i18n("Calculating..."));
    m_ui->lblSHA256->setText(i18n("Calculating..."));

    showChecksum(QCryptographicHash::Md5, archive->fileName(), m_ui->lblMD5);
    showChecksum(QCryptographicHash::Sha1, archive->fileName(), m_ui->lblSHA1);
    showChecksum(QCryptographicHash::Sha256, archive->fileName(), m_ui->lblSHA256);

    connect(m_ui->buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
}

QString PropertiesDialog::calcHash(QCryptographicHash::Algorithm algorithm, const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return QString();
    }

    QCryptographicHash hash(algorithm);
    hash.addData(&file);

    return QString::fromLatin1(hash.result().toHex());
}

void PropertiesDialog::showChecksum(QCryptographicHash::Algorithm algorithm, const QString &fileName, QLabel *label)
{
    // Calculate checksum in another thread.
    auto futureWatcher = new QFutureWatcher<QString>(this);
    connect(futureWatcher, &QFutureWatcher<QString>::finished, this, [=]() {
        label->setText(futureWatcher->result());
        futureWatcher->deleteLater();
    });

    auto future = QtConcurrent::run(this, &PropertiesDialog::calcHash, algorithm, fileName);
    futureWatcher->setFuture(future);
}

#include "propertiesdialog.moc"
}

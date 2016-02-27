/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009,2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
 * Copyright (C) 2015 Elvis Angelaccio <elvis.angelaccio@kdemail.net>
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

#include "createdialog.h"
#include "ark_debug.h"
#include "ui_createdialog.h"
#include "kerfuffle/archive_kerfuffle.h"

#include <KFileWidget>
#include <KMessageBox>
#include <KSharedConfig>
#include <KUrlComboBox>
#include <KWindowConfig>

#include <QDebug>
#include <QMimeDatabase>
#include <QPushButton>
#include <QScreen>
#include <QUrl>
#include <QWindow>

namespace Kerfuffle
{
class CreateDialogUI: public QWidget, public Ui::CreateDialog
{
public:
    CreateDialogUI(QWidget *parent = 0)
            : QWidget(parent) {
        setupUi(this);
    }
};

CreateDialog::CreateDialog(QWidget *parent,
                           const QString &caption,
                           const QUrl &startDir)
        : QDialog(parent, Qt::Dialog)
{
    qCDebug(ARK) << "CreateDialog loaded";

    this->setWindowTitle(caption);

    m_vlayout = new QVBoxLayout();
    setLayout(m_vlayout);

    m_fileWidget = new KFileWidget(startDir, this);
    m_vlayout->addWidget(m_fileWidget);

    m_fileWidget->setMode(KFile::File | KFile::LocalOnly);
    m_fileWidget->setConfirmOverwrite(true);
    m_fileWidget->setOperationMode(KFileWidget::Saving);
    m_fileWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);

    connect(m_fileWidget->okButton(), &QPushButton::clicked, this, &CreateDialog::slotOkButtonClicked);
    connect(m_fileWidget, &KFileWidget::accepted, m_fileWidget, &KFileWidget::accept);
    connect(m_fileWidget, &KFileWidget::accepted, this, &CreateDialog::accept);
    m_fileWidget->okButton()->show();

    m_fileWidget->cancelButton()->show();
    connect(m_fileWidget->cancelButton(), &QPushButton::clicked, this, &QDialog::reject);

    loadConfiguration();

    connect(this, &QDialog::accepted, this, &CreateDialog::updateDefaultMimeType);
    connect(this, &QDialog::finished, this, &CreateDialog::slotSaveWindowSize);
    connect(m_fileWidget, &KFileWidget::filterChanged, this, &CreateDialog::updateDisplayedOptions);

    m_ui = new CreateDialogUI(this);
    m_ui->groupEncryptionOptions->hide();
    m_vlayout->addWidget(m_ui);

    connect(m_ui->encryptCheckBox, &QCheckBox::toggled, this, &CreateDialog::encryptionToggled);
    connect(m_ui->showPwdCheckbox, &QCheckBox::toggled, this, &CreateDialog::showPasswordToggled);
}

QSize CreateDialog::sizeHint() const
{
    // Used only when no previous window size has been stored
    return QSize(750,450);
}

QList<QUrl> CreateDialog::selectedUrls() const
{
    return m_fileWidget->selectedUrls();
}

QString CreateDialog::currentMimeFilter() const
{
    return m_fileWidget->currentMimeFilter();
}

QString CreateDialog::password() const
{
    return m_ui->pwdInput->text();
}

bool CreateDialog::isHeaderEncryptionChecked() const
{
    return (m_ui->encryptHeaderCheckBox->isEnabled() && m_ui->encryptHeaderCheckBox->isChecked());
}

void CreateDialog::accept()
{
    if ((m_ui->pwdInput->text() == m_ui->pwdConfirmInput->text()) || m_ui->showPwdCheckbox->isChecked()) {
        QDialog::accept();
    } else {
        KMessageBox::error(NULL, i18n("The chosen password does not match the given verification password."));
        m_ui->pwdInput->clear();
        m_ui->pwdConfirmInput->clear();
    }
}

void CreateDialog::restoreWindowSize()
{
    // Restore window size from config file, needs a windowHandle so must be called after show()
    KConfigGroup group(KSharedConfig::openConfig(), "CreateDialog");
    //KWindowConfig::restoreWindowSize(windowHandle(), group);
    //KWindowConfig::restoreWindowSize is broken atm., so we need this hack:
    const QRect desk = windowHandle()->screen()->geometry();
    this->resize(QSize(group.readEntry(QString::fromLatin1("Width %1").arg(desk.width()), windowHandle()->size().width()),
                     group.readEntry(QString::fromLatin1("Height %1").arg(desk.height()), windowHandle()->size().height())));
}

void CreateDialog::slotSaveWindowSize()
{
    // Save dialog window size
    KConfigGroup group(KSharedConfig::openConfig(), "CreateDialog");
    KWindowConfig::saveWindowSize(windowHandle(), group, KConfigBase::Persistent);
}

void CreateDialog::slotOkButtonClicked()
{
    // In case the user tries to leave the lineEdit empty:
    if (m_fileWidget->locationEdit()->lineEdit()->text().isEmpty()) {
        KMessageBox::sorry(this, i18n("Please select a filename for the archive."), i18n("No file selected"));
        return;
    }
    // This slot sets the url from text in the lineEdit, asks for overwrite etc, and emits signal accepted
    m_fileWidget->slotOk();
}

void CreateDialog::encryptionToggled(bool checked)
{
    m_ui->groupEncryptionOptions->setVisible(checked);
}

void CreateDialog::showPasswordToggled(bool checked)
{
    if (checked) {
        m_ui->pwdConfirmInputLabel->hide();
        m_ui->pwdConfirmInput->hide();
        m_ui->pwdInput->setEchoMode(QLineEdit::Normal);
    } else {
        m_ui->pwdConfirmInputLabel->show();
        m_ui->pwdConfirmInput->show();
        m_ui->pwdInput->setEchoMode(QLineEdit::Password);
    }
}

void CreateDialog::updateDefaultMimeType()
{
    m_config.writeEntry("LastMimeType", m_fileWidget->currentFilterMimeType().name());
}

void CreateDialog::updateDisplayedOptions(const QString &filter)
{
    qCDebug(ARK) << "Current selected mime filter: " << filter;

    if (Kerfuffle::supportedEncryptEntriesMimeTypes().contains(filter)) {
        m_ui->encryptCheckBox->setEnabled(true);
        m_ui->encryptCheckBox->setToolTip(QString());
        m_ui->groupEncryptionOptions->setEnabled(true);
    } else {
        m_ui->encryptCheckBox->setEnabled(false);
        m_ui->encryptCheckBox->setToolTip(i18n("Protection of the archive with password is not possible with the %1 format.",
                                               QMimeDatabase().mimeTypeForName(filter).comment()));
        m_ui->groupEncryptionOptions->setEnabled(false);
    }

    if (Kerfuffle::supportedEncryptHeaderMimeTypes().contains(filter)) {
        m_ui->encryptHeaderCheckBox->setEnabled(true);
        m_ui->encryptHeaderCheckBox->setToolTip(QString());
    } else {
        m_ui->encryptHeaderCheckBox->setEnabled(false);
        // show the tooltip only if the whole group is enabled
        if (m_ui->groupEncryptionOptions->isEnabled()) {
            m_ui->encryptHeaderCheckBox->setToolTip(i18n("Protection of the list of files is not possible with the %1 format.",
                                                         QMimeDatabase().mimeTypeForName(filter).comment()));
        } else {
             m_ui->encryptHeaderCheckBox->setToolTip(QString());
        }
    }
}

void CreateDialog::loadConfiguration()
{
    m_config = KConfigGroup(KSharedConfig::openConfig()->group("CreateDialog"));

    const QString defaultMimeType = QStringLiteral("application/x-compressed-tar");
    const QString lastMimeType = m_config.readEntry("LastMimeType", defaultMimeType);
    QStringList writeMimeTypes = Kerfuffle::supportedWriteMimeTypes().toList();

    // The filters need to be sorted by comment, so create a QMap with
    // comment as key (QMaps are always sorted by key) and QMimeType
    // as value. Then convert the QMap back to a QStringList. Mimetypes
    // with empty comments are discarded.
    QMimeDatabase db;
    QMap<QString,QMimeType> mimeMap;
    foreach (const QString &s, writeMimeTypes) {
        QMimeType mime(db.mimeTypeForName(s));
        if (!mime.comment().isEmpty()) {
            mimeMap[mime.comment()] = mime;
        }
    }

    writeMimeTypes.clear();

    QMapIterator<QString,QMimeType> j(mimeMap);
    while (j.hasNext()) {
        j.next();
        writeMimeTypes << j.value().name();
    }

    if (writeMimeTypes.contains(lastMimeType)) {
        m_fileWidget->setMimeFilter(writeMimeTypes, lastMimeType);
    } else {
        m_fileWidget->setMimeFilter(writeMimeTypes, defaultMimeType);
    }
}

}

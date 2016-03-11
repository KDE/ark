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

#include <KColorScheme>
#include <KFileWidget>
#include <KMessageBox>
#include <KSharedConfig>
#include <KUrlComboBox>
#include <KWindowConfig>
#include <KFileFilterCombo>

#include <QDebug>
#include <QLineEdit>
#include <QMimeDatabase>
#include <QPushButton>
#include <QUrl>

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

ArchiveTypeFilter::ArchiveTypeFilter(const QMimeType &newMimeType,
                                     const QStringList &newGlobPatterns,
                                     const QString &newComment) :
    mimeType(newMimeType),
    globPatterns(newGlobPatterns),
    comment(newComment)
{
}

CreateDialog::CreateDialog(QWidget *parent,
                           const QString &caption,
                           const QUrl &startDir)
        : QDialog(parent, Qt::Dialog)
{
    qCDebug(ARK) << "CreateDialog loaded";

    setWindowTitle(caption);
    setModal(true);

    m_vlayout = new QVBoxLayout();
    setLayout(m_vlayout);
    m_fileWidget = new KFileWidget(startDir, this);
    m_vlayout->addWidget(m_fileWidget);

    m_fileWidget->setMode(KFile::File | KFile::LocalOnly);
    m_fileWidget->setConfirmOverwrite(true);
    m_fileWidget->setOperationMode(KFileWidget::Saving);
    m_fileWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::MinimumExpanding);
    m_fileWidget->setFilter(filterFromMimeTypes(Kerfuffle::supportedWriteMimeTypes()));
    m_fileWidget->filterWidget()->setEditable(false);
    m_fileWidget->okButton()->show();
    m_fileWidget->cancelButton()->show();

    connect(m_fileWidget->okButton(), &QPushButton::clicked, this, &CreateDialog::slotOkButtonClicked);
    connect(m_fileWidget, &KFileWidget::accepted, m_fileWidget, &KFileWidget::accept);
    connect(m_fileWidget, &KFileWidget::accepted, this, &CreateDialog::accept);
    connect(m_fileWidget->cancelButton(), &QPushButton::clicked, this, &QDialog::reject);
    connect(this, &QDialog::accepted, this, &CreateDialog::slotUpdateDefaultMimeType);
    connect(this, &QDialog::finished, this, &CreateDialog::slotSaveWindowSize);
    // The currentIndexChanged signal is overloaded and can send both an int and QString, hence the complex syntax.
    connect(m_fileWidget->filterWidget(), static_cast<void(KFileFilterCombo::*)(int)>(&KFileFilterCombo::currentIndexChanged), this, &CreateDialog::slotFilterChanged);

    m_ui = new CreateDialogUI(this);
    m_ui->groupEncryptionOptions->hide();
    KColorScheme colorScheme(QPalette::Active, KColorScheme::View);
    m_ui->pwdWidget->setBackgroundWarningColor(colorScheme.background(KColorScheme::NegativeBackground).color());
    m_ui->pwdWidget->setAllowEmptyPasswords(false);
    m_ui->pwdWidget->setPasswordStrengthMeterVisible(false);

    connect(m_ui->encryptCheckBox, &QCheckBox::toggled, this, &CreateDialog::slotEncryptionToggled);

    m_vlayout->addWidget(m_ui);

    loadConfiguration();
}

QSize CreateDialog::sizeHint() const
{
    // Used only when no previous window size has been stored.
    return QSize(750,450);
}

QList<QUrl> CreateDialog::selectedUrls() const
{
    return m_fileWidget->selectedUrls();
}

QString CreateDialog::password() const
{
    return m_ui->pwdWidget->password();
}

bool CreateDialog::isEncryptionAvailable() const
{
    return m_ui->encryptCheckBox->isEnabled();
}

bool CreateDialog::isEncryptionEnabled() const
{
    return isEncryptionAvailable() && m_ui->encryptCheckBox->isChecked() && m_ui->groupEncryptionOptions->isEnabled();
}

bool CreateDialog::isHeaderEncryptionAvailable() const
{
    return isEncryptionEnabled() && m_ui->encryptHeaderCheckBox->isEnabled();
}

bool CreateDialog::isHeaderEncryptionEnabled() const
{
    return isHeaderEncryptionAvailable() && m_ui->encryptHeaderCheckBox->isChecked();
}

void CreateDialog::accept()
{
    if (!isEncryptionEnabled()) {
        QDialog::accept();
        return;
    }

    switch (m_ui->pwdWidget->passwordStatus()) {
    case KNewPasswordWidget::WeakPassword:
    case KNewPasswordWidget::StrongPassword:
        QDialog::accept();
        break;
    case KNewPasswordWidget::PasswordNotVerified:
        KMessageBox::error(Q_NULLPTR, i18n("The chosen password does not match the given verification password."));
        break;
    case KNewPasswordWidget::EmptyPasswordNotAllowed:
        KMessageBox::error(Q_NULLPTR, i18n("The password cannot be empty."));
        break;
    default:
        break;
    }
}

void CreateDialog::slotFilterChanged(int index)
{
    QMimeType currentMimeType = m_filterList.at(index).mimeType;

    qCDebug(ARK) << "Filter changed to: " << currentMimeType.name();

    if (Kerfuffle::supportedEncryptEntriesMimeTypes().contains(currentMimeType.name())) {
        m_ui->encryptCheckBox->setEnabled(true);
        m_ui->encryptCheckBox->setToolTip(QString());
        m_ui->groupEncryptionOptions->setEnabled(true);
    } else {
        m_ui->encryptCheckBox->setEnabled(false);
        m_ui->encryptCheckBox->setToolTip(i18n("Protection of the archive with password is not possible with the %1 format.",
                                               currentMimeType.comment()));
        m_ui->groupEncryptionOptions->setEnabled(false);
    }

    if (Kerfuffle::supportedEncryptHeaderMimeTypes().contains(currentMimeType.name())) {
        m_ui->encryptHeaderCheckBox->setEnabled(true);
        m_ui->encryptHeaderCheckBox->setToolTip(QString());
    } else {
        m_ui->encryptHeaderCheckBox->setEnabled(false);
        // Show the tooltip only if the encryption is still enabled.
        // This is needed because if the new filter is e.g. tar, the whole encryption group gets disabled.
        if (isEncryptionEnabled()) {
            m_ui->encryptHeaderCheckBox->setToolTip(i18n("Protection of the list of files is not possible with the %1 format.",
                                                         currentMimeType.comment()));
        } else {
            m_ui->encryptHeaderCheckBox->setToolTip(QString());
        }
    }
}

void CreateDialog::restoreWindowSize()
{
    // Restore window size from config file, needs a windowHandle so must be called after show().
    KConfigGroup group(KSharedConfig::openConfig(), "CreateDialog");
    KWindowConfig::restoreWindowSize(windowHandle(), group);
}

void CreateDialog::slotSaveWindowSize()
{
    // Save dialog window size.
    KConfigGroup group(KSharedConfig::openConfig(), "CreateDialog");
    KWindowConfig::saveWindowSize(windowHandle(), group, KConfigBase::Persistent);
}

void CreateDialog::slotOkButtonClicked()
{
    // In case the user tries to leave the lineEdit empty.
    if (m_fileWidget->locationEdit()->lineEdit()->text().isEmpty()) {
        KMessageBox::sorry(this, i18n("Please select a filename for the archive."), i18n("No file selected"));
        return;
    }
    // This slot sets the url from text in the lineEdit, asks for overwrite etc, and emits signal accepted.
    m_fileWidget->slotOk();
}

void CreateDialog::slotEncryptionToggled(bool checked)
{
    m_ui->groupEncryptionOptions->setVisible(checked);
}

void CreateDialog::slotUpdateDefaultMimeType()
{
    m_config.writeEntry("LastMimeType", currentFilterMimeType().name());
}

void CreateDialog::loadConfiguration()
{
    m_config = KConfigGroup(KSharedConfig::openConfig()->group("CreateDialog"));

    // Read the last used mimetype from config file, use tar.gz in case the read mimetype is invalid.
    setCurrentFilterFromMimeType(m_config.readEntry("LastMimeType", QStringLiteral("application/x-compressed-tar")));
}

void CreateDialog::setCurrentFilterFromMimeType(const QString &mimeType)
{
    QMimeDatabase db;
    const QMimeType defaultMimeType = db.mimeTypeForName(QStringLiteral("application/x-compressed-tar"));
    const QMimeType mimeTypeToSet = db.mimeTypeForName(mimeType);

    qCDebug(ARK) << "Setting filter to: " << mimeType;

    // Find the index for mimetype and select corresponding filter.
    // Use tar.gz in case mimetype is not in m_filterList.
    if (m_filterList.contains(ArchiveTypeFilter(mimeTypeToSet))) {
        m_fileWidget->filterWidget()->setCurrentIndex(m_filterList.indexOf(ArchiveTypeFilter(mimeTypeToSet)));
    } else if (m_filterList.contains(ArchiveTypeFilter(defaultMimeType))) {
        m_fileWidget->filterWidget()->setCurrentIndex(m_filterList.indexOf(ArchiveTypeFilter(defaultMimeType)));
    }
}

QMimeType CreateDialog::currentFilterMimeType() const
{
    return m_filterList.at(m_fileWidget->filterWidget()->currentIndex()).mimeType;
}

QString CreateDialog::filterFromMimeTypes(const QSet<QString> &mimeTypes)
{
    // Populate m_filterList from QSet. m_filterList is needed to be able to
    // modify the mimetypes.
    foreach (const QString &s, mimeTypes.toList()) {

        QMimeType mime(QMimeDatabase().mimeTypeForName(s));

        if (mime.comment().isEmpty()) {
            continue;
        }

        m_filterList.append(ArchiveTypeFilter(mime,
                                              mime.globPatterns(),
                                              mime.comment()));

        // There is no mimetype for tar.lz although libarchive will create this
        // archive type when the compression is lzip. Therefore we need to modify
        // the comment and glob pattern for application/x-lzip.
        if (mime.name() == QLatin1String("application/x-lzip")) {
            m_filterList.last().comment = QStringLiteral("Tar archive (lzip compressed)");
            m_filterList.last().globPatterns.prepend(QStringLiteral("*.tar.lz"));
        }
    }

    // Sort the filterlist by comment.
    qSort(m_filterList);

    // Construct a combined filterstring by iterating through m_filterList.
    // The string can be used as argument for KFileWidget::setFilter().
    QString combinedFilter;
    foreach (const ArchiveTypeFilter &filter, m_filterList) {
        foreach (const QString &glob, filter.globPatterns) {
            combinedFilter += glob + QLatin1Char(' ');
        }
        combinedFilter.chop(1);
        combinedFilter += QStringLiteral("|%1 (%2)\n").arg(filter.comment, filter.globPatterns.at(0));
    }
    combinedFilter.chop(1);

    return combinedFilter;
}

}

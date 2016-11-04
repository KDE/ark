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

#include "compressionoptionswidget.h"
#include "ark_debug.h"
#include "archiveformat.h"
#include "pluginmanager.h"

#include <KColorScheme>
#include <KPluginMetaData>

#include <QMimeDatabase>

namespace Kerfuffle
{
CompressionOptionsWidget::CompressionOptionsWidget(QWidget *parent,
                                                   const CompressionOptions &opts)
    : QWidget(parent)
    , m_opts(opts)
{
    setupUi(this);

    KColorScheme colorScheme(QPalette::Active, KColorScheme::View);
    pwdWidget->setBackgroundWarningColor(colorScheme.background(KColorScheme::NegativeBackground).color());
    pwdWidget->setPasswordStrengthMeterVisible(false);

    connect(multiVolumeCheckbox, &QCheckBox::stateChanged, this, &CompressionOptionsWidget::slotMultiVolumeChecked);

    if (m_opts.isVolumeSizeSet()) {
        multiVolumeCheckbox->setChecked(true);
        // Convert from kilobytes.
        volumeSizeSpinbox->setValue(static_cast<double>(m_opts.volumeSize()) / 1024);
    }
}

CompressionOptions CompressionOptionsWidget::commpressionOptions() const
{
    CompressionOptions opts;
    opts.setCompressionLevel(compLevelSlider->value());
    if (multiVolumeCheckbox->isChecked()) {
        opts.setVolumeSize(volumeSize());
    }
    if (!compMethodComboBox->currentText().isEmpty()) {
        opts.setCompressionMethod(compMethodComboBox->currentText());
    }

    return opts;
}

int CompressionOptionsWidget::compressionLevel() const
{
    if (compLevelSlider->isEnabled()) {
        return compLevelSlider->value();
    } else {
        return -1;
    }
}

QString CompressionOptionsWidget::compressionMethod() const
{
    return compMethodComboBox->currentText();
}

ulong CompressionOptionsWidget::volumeSize() const
{
    if (collapsibleMultiVolume->isEnabled() && multiVolumeCheckbox->isChecked()) {
        // Convert to kilobytes.
        return volumeSizeSpinbox->value() * 1024;
    } else {
        return 0;
    }
}

void CompressionOptionsWidget::setEncryptionVisible(bool visible)
{
    collapsibleEncryption->setVisible(visible);
}

QString CompressionOptionsWidget::password() const
{
    return pwdWidget->password();
}

void CompressionOptionsWidget::updateWidgets()
{
    const KPluginMetaData metadata = PluginManager().preferredPluginFor(m_mimetype)->metaData();
    const ArchiveFormat archiveFormat = ArchiveFormat::fromMetadata(m_mimetype, metadata);
    Q_ASSERT(archiveFormat.isValid());

    if (archiveFormat.encryptionType() != Archive::Unencrypted) {
        collapsibleEncryption->setEnabled(true);
        collapsibleEncryption->setToolTip(QString());
        pwdWidget->setEnabled(true);

        if (archiveFormat.encryptionType() == Archive::HeaderEncrypted) {
            encryptHeaderCheckBox->setEnabled(true);
            encryptHeaderCheckBox->setToolTip(QString());
        } else {
            encryptHeaderCheckBox->setEnabled(false);
            // Show the tooltip only if the encryption is still enabled.
            // This is needed because if the new filter is e.g. tar, the whole encryption group gets disabled.
            if (collapsibleEncryption->isEnabled() && collapsibleEncryption->isExpanded()) {
                encryptHeaderCheckBox->setToolTip(i18n("Protection of the list of files is not possible with the %1 format.",
                                                       m_mimetype.comment()));
            } else {
                encryptHeaderCheckBox->setToolTip(QString());
            }
        }

    } else {
        collapsibleEncryption->setEnabled(false);
        collapsibleEncryption->setToolTip(i18n("Protection of the archive with password is not possible with the %1 format.",
                                               m_mimetype.comment()));
        pwdWidget->setEnabled(false);
        encryptHeaderCheckBox->setToolTip(QString());
    }

    collapsibleCompression->setEnabled(true);
    if (archiveFormat.maxCompressionLevel() == 0) {
        compLevelSlider->setEnabled(false);
        lblCompLevel1->setEnabled(false);
        lblCompLevel2->setEnabled(false);
        lblCompLevel3->setEnabled(false);
        compLevelSlider->setToolTip(i18n("It is not possible to set compression level for the %1 format.",
                                                m_mimetype.comment()));
    } else {
        compLevelSlider->setEnabled(true);
        lblCompLevel1->setEnabled(true);
        lblCompLevel2->setEnabled(true);
        lblCompLevel3->setEnabled(true);
        compLevelSlider->setToolTip(QString());
        compLevelSlider->setMinimum(archiveFormat.minCompressionLevel());
        compLevelSlider->setMaximum(archiveFormat.maxCompressionLevel());
        if (m_opts.isCompressionLevelSet()) {
            compLevelSlider->setValue(m_opts.compressionLevel());
        } else {
            compLevelSlider->setValue(archiveFormat.defaultCompressionLevel());
        }
    }

    if (archiveFormat.compressionMethods().isEmpty()) {
        lblCompMethod->setEnabled(false);
        compMethodComboBox->setEnabled(false);
        compMethodComboBox->setToolTip(i18n("It is not possible to set compression method for the %1 format.",
                                            m_mimetype.comment()));
        compMethodComboBox->clear();
    } else {
        lblCompMethod->setEnabled(true);
        compMethodComboBox->setEnabled(true);
        compMethodComboBox->setToolTip(QString());
        compMethodComboBox->clear();
        compMethodComboBox->insertItems(0, archiveFormat.compressionMethods().keys());
        if (!m_opts.compressionMethod().isEmpty() &&
            compMethodComboBox->findText(m_opts.compressionMethod()) > -1) {
            compMethodComboBox->setCurrentText(m_opts.compressionMethod());
        } else {
            compMethodComboBox->setCurrentText(archiveFormat.defaultCompressionMethod());
        }
    }
    collapsibleCompression->setEnabled(compLevelSlider->isEnabled() || compMethodComboBox->isEnabled());

    if (archiveFormat.supportsMultiVolume()) {
        collapsibleMultiVolume->setEnabled(true);
        collapsibleMultiVolume->setToolTip(QString());
    } else {
        collapsibleMultiVolume->setEnabled(false);
        collapsibleMultiVolume->setToolTip(i18n("The %1 format does not support multi-volume archives.",
                                                m_mimetype.comment()));
    }
}

void CompressionOptionsWidget::setMimeType(const QMimeType &mimeType)
{
    m_mimetype = mimeType;
    updateWidgets();
}

bool CompressionOptionsWidget::isEncryptionAvailable() const
{
    return collapsibleEncryption->isEnabled();
}

bool CompressionOptionsWidget::isEncryptionEnabled() const
{
    return isEncryptionAvailable() && collapsibleEncryption->isExpanded();
}

bool CompressionOptionsWidget::isHeaderEncryptionAvailable() const
{
    return isEncryptionEnabled() && encryptHeaderCheckBox->isEnabled();
}

bool CompressionOptionsWidget::isHeaderEncryptionEnabled() const
{
    return isHeaderEncryptionAvailable() && encryptHeaderCheckBox->isChecked();
}

KNewPasswordWidget::PasswordStatus CompressionOptionsWidget::passwordStatus() const
{
    return pwdWidget->passwordStatus();
}

void CompressionOptionsWidget::slotMultiVolumeChecked(int state)
{
    if (state == Qt::Checked) {
        lblVolumeSize->setEnabled(true);
        volumeSizeSpinbox->setEnabled(true);
    } else {
        lblVolumeSize->setEnabled(false);
        volumeSizeSpinbox->setEnabled(false);
    }
}

}

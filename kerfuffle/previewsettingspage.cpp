/*
    SPDX-FileCopyrightText: 2015 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "previewsettingspage.h"

namespace Kerfuffle
{
PreviewSettingsPage::PreviewSettingsPage(QWidget *parent, const QString &name, const QString &iconName)
    : SettingsPage(parent, name, iconName)
{
    setupUi(this);
    connect(kcfg_limitPreviewFileSize, &QCheckBox::toggled, this, &PreviewSettingsPage::slotToggled);
}

void PreviewSettingsPage::slotToggled(bool enabled)
{
    kcfg_previewFileSizeLimit->setEnabled(enabled);
}
}


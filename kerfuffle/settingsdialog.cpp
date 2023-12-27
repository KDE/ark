/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "settingsdialog.h"

namespace Kerfuffle
{
SettingsDialog::SettingsDialog(QWidget *parent, const QString &name, KCoreConfigSkeleton *config)
    : KConfigDialog(parent, name, config)
{
}

void SettingsDialog::updateWidgetsDefault()
{
    Q_EMIT defaultsButtonClicked();
}

}

#include "moc_settingsdialog.cpp"

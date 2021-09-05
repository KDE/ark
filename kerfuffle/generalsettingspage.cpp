/*
    ark -- archiver for the KDE project

    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "generalsettingspage.h"

namespace Kerfuffle
{
GeneralSettingsPage::GeneralSettingsPage(QWidget *parent, const QString &name, const QString &iconName)
    : SettingsPage(parent, name, iconName)
{
    setupUi(this);
}
}


/*
    ark -- archiver for the KDE project

    SPDX-FileCopyrightText: 2015 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "extractionsettingspage.h"

namespace Kerfuffle
{
ExtractionSettingsPage::ExtractionSettingsPage(QWidget *parent, const QString &name, const QString &iconName)
    : SettingsPage(parent, name, iconName)
{
    setupUi(this);
}
}


/*
    ark -- archiver for the KDE project

    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef GENERALSETTINGSPAGE_H
#define GENERALSETTINGSPAGE_H

#include "settingspage.h"
#include "ui_generalsettingspage.h"

namespace Kerfuffle
{
class KERFUFFLE_EXPORT GeneralSettingsPage : public SettingsPage, public Ui::GeneralSettingsPage
{
    Q_OBJECT

public:
    explicit GeneralSettingsPage(QWidget *parent = nullptr, const QString &name = QString(), const QString &iconName = QString());
};
}

#endif

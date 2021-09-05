/*
    SPDX-FileCopyrightText: 2015 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef EXTRACTIONSETTINGSPAGE_H
#define EXTRACTIONSETTINGSPAGE_H

#include "settingspage.h"
#include "ui_extractionsettingspage.h"

namespace Kerfuffle
{
class KERFUFFLE_EXPORT ExtractionSettingsPage : public SettingsPage, public Ui::ExtractionSettingsPage
{
    Q_OBJECT

public:
    explicit ExtractionSettingsPage(QWidget *parent = nullptr, const QString &name = QString(), const QString &iconName = QString());
};
}

#endif

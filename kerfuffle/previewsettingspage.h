/*
    SPDX-FileCopyrightText: 2015 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef PREVIEWSETTINGSPAGE_H
#define PREVIEWSETTINGSPAGE_H

#include "settingspage.h"
#include "ui_previewsettingspage.h"

namespace Kerfuffle
{
class KERFUFFLE_EXPORT PreviewSettingsPage : public SettingsPage, public Ui::PreviewSettingsPage
{
    Q_OBJECT

public:
    explicit PreviewSettingsPage(QWidget *parent = nullptr, const QString &name = QString(), const QString &iconName = QString());

private Q_SLOTS:
    void slotToggled(bool enabled);
};
}

#endif

/*
    SPDX-FileCopyrightText: 2015 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "settingspage.h"

namespace Kerfuffle
{
SettingsPage::SettingsPage(QWidget *parent, const QString &name, const QString &iconName)
    : QWidget(parent),
      m_name(name),
      m_iconName(iconName)
{}

QString SettingsPage::name() const
{
    return m_name;
}

QString SettingsPage::iconName() const
{
    return m_iconName;
}

void SettingsPage::slotSettingsChanged()
{
}

void SettingsPage::slotDefaultsButtonClicked()
{
}

}

#include "moc_settingspage.cpp"

/*
    SPDX-FileCopyrightText: 2015 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include "kerfuffle_export.h"

#include <QWidget>

namespace Kerfuffle
{
class KERFUFFLE_EXPORT SettingsPage : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPage(QWidget *parent = nullptr, const QString &name = QString(), const QString &iconName = QString());

    QString name() const;
    QString iconName() const;

public Q_SLOTS:
    virtual void slotSettingsChanged();
    virtual void slotDefaultsButtonClicked();

private:
    QString m_name;
    QString m_iconName;
};
}

#endif

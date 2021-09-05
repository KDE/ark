/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include "kerfuffle_export.h"

#include <KConfigDialog>

namespace Kerfuffle
{

/**
 * A custom KConfigDialog that emits a signal when the Default button has been clicked.
 */
class KERFUFFLE_EXPORT SettingsDialog : public KConfigDialog
{
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget *parent, const QString &name, KCoreConfigSkeleton *config);

Q_SIGNALS:
    void defaultsButtonClicked();

protected Q_SLOTS:
    void updateWidgetsDefault() override;

};

}

#endif

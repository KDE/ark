/*
    SPDX-FileCopyrightText: 2021 Jiří Wolker <woljiri@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later

*/

#ifndef WELCOMESCREEN_H
#define WELCOMESCREEN_H

#include "ui_welcomescreen.h"


class WelcomeScreen: public QWidget, Ui::WelcomeScreen
{
    Q_OBJECT
public:
    explicit WelcomeScreen(QWidget *parent = nullptr);
    ~WelcomeScreen() override;

Q_SIGNALS:
    void openClicked();
    void newClicked();
};

#endif // WELCOMESCREEN_H


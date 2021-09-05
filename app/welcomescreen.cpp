/*
    ark -- archiver for the KDE project

    SPDX-FileCopyrightText: 2021 Jiří Wolker <woljiri@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later

*/

#include "welcomescreen.h"

WelcomeScreen::WelcomeScreen(QWidget *parent)
        : QWidget(parent)
{
    Q_ASSERT(parent);

    setupUi(this);

    appIcon->setPixmap(QIcon::fromTheme(QStringLiteral("utilities-file-archiver")).pixmap(128));

    connect(openButton, &QPushButton::clicked, this, &WelcomeScreen::openClicked);
    connect(newButton, &QPushButton::clicked, this, &WelcomeScreen::newClicked);
}

WelcomeScreen::~WelcomeScreen()
{
}

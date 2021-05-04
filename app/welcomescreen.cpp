/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2021 Jiří Wolker <woljiri@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
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

/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
    SPDX-FileCopyrightText: 2021 Jiří Wolker <woljiri@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <KParts/MainWindow>
#include <KParts/OpenUrlArguments>
#include <QStackedWidget>

#include "welcomeview/welcomeview.h"

namespace KParts
{
class ReadWritePart;
}

class KRecentFilesMenu;

class MainWindow: public KParts::MainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    bool loadPart();
    KRecentFilesMenu* recentFilesMenu() const;

    void dragEnterEvent(class QDragEnterEvent * event) override;
    void dropEvent(class QDropEvent * event) override;
    void dragMoveEvent(class QDragMoveEvent * event) override;

public Q_SLOTS:
    void openUrl(const QUrl &url);
    void setShowExtractDialog(bool);

    void showWelcomeScreen(bool force = false);
    void hideWelcomeScreen();

protected:
    void closeEvent(QCloseEvent *event) override;
    QSize sizeHint() const override;

private Q_SLOTS:
    void updateActions();
    void newArchive();
    void openArchive();
    void quit();
    void showSettings();
    void writeSettings();
    void addPartUrl();

private:
    void setupActions();
    void updateHamburgerMenu();

    KParts::ReadWritePart *m_part;
    KRecentFilesMenu      *m_recentFilesMenu;
    QAction               *m_openAction;
    QAction               *m_newAction;
    KParts::OpenUrlArguments m_openArgs;
    WelcomeView           *m_welcomeView;
    QStackedWidget        *m_windowContents;
};

#endif // MAINWINDOW_H

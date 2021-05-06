/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (C) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <KParts/MainWindow>
#include <KParts/OpenUrlArguments>

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

    void dragEnterEvent(class QDragEnterEvent * event) override;
    void dropEvent(class QDropEvent * event) override;
    void dragMoveEvent(class QDragMoveEvent * event) override;

public Q_SLOTS:
    void openUrl(const QUrl &url);
    void setShowExtractDialog(bool);

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

    KParts::ReadWritePart *m_part;
    KRecentFilesMenu      *m_recentFilesMenu;
    QAction               *m_openAction;
    QAction               *m_newAction;
    KParts::OpenUrlArguments m_openArgs;
};

#endif // MAINWINDOW_H

/*
 * ark: A program for modifying archives via a GUI.
 *
 * Copyright (C) 2004-2008, Henrique Pinto <henrique.pinto@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#ifndef ARKVIEWER_H
#define ARKVIEWER_H

#include <KParts/BrowserExtension>
#include <KParts/ReadOnlyPart>
#include <KService>

#include <QDialog>
#include <QWeakPointer>
#include <QMimeType>
#include <QVBoxLayout>

class ArkViewer : public QDialog
{
    Q_OBJECT

public:
    virtual ~ArkViewer();
    QSize sizeHint() const Q_DECL_OVERRIDE;

    static void view(const QString& fileName, QWidget* parent = 0);

private slots:
    void dialogClosed();

private:
    explicit ArkViewer(QWidget* parent = 0, Qt::WindowFlags flags = 0);

    static KService::Ptr getViewer(const QString& mimeType);
    bool viewInInternalViewer(const QString& fileName, const QMimeType& mimeType);

    QPointer<KParts::ReadOnlyPart> m_part;
    QVBoxLayout *m_mainLayout;
};

#endif // ARKVIEWER_H


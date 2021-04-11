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

#include "ui_arkviewer.h"

#include <KParts/MainWindow>
#include <KService>

namespace KParts
{
    class ReadOnlyPart;
}

#include <QMimeType>

#include <memory>

class ArkViewer : public KParts::MainWindow, public Ui::ArkViewer
{
    Q_OBJECT

public:
    ~ArkViewer() override;

    static void view(const QString& fileName);

private:
    explicit ArkViewer();

    static KService::Ptr getExternalViewer(const QString& mimeType);
    static KPluginMetaData getInternalViewer(const QString &mimeType);

    static void openExternalViewer(const KService::Ptr viewer, const QString& fileName);
    static void openInternalViewer(const KPluginMetaData &pluginMetaData, const QString &fileName, const QMimeType &mimeType);

    static bool askViewAsPlainText(const QMimeType& mimeType);

    bool viewInInternalViewer(const KPluginMetaData &pluginMetaData, const QString &fileName, const QMimeType &mimeType);

private Q_SLOTS:
    void aboutKPart();

private:
    std::unique_ptr<KParts::ReadOnlyPart> m_part;
    QString m_fileName;
};

#endif // ARKVIEWER_H

/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>
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

#include "compressfileitemaction.h"

#include <QMenu>
#include <QMimeDatabase>

#include <KPluginFactory>
#include <KLocalizedString>
#include <KRun>

#include "pluginmanager.h"

K_PLUGIN_FACTORY_WITH_JSON(CompressFileItemActionFactory, "compressfileitemaction.json", registerPlugin<CompressFileItemAction>();)

using namespace Kerfuffle;

CompressFileItemAction::CompressFileItemAction(QObject* parent, const QVariantList&)
    : KAbstractFileItemActionPlugin(parent)
    , m_pluginManager(new PluginManager(this))
{}

QList<QAction*> CompressFileItemAction::actions(const KFileItemListProperties& fileItemInfos, QWidget* parentWidget)
{
    // #268163: don't offer compression on already compressed archives.
    if (m_pluginManager->supportedMimeTypes().contains(fileItemInfos.mimeType())) {
        return {};
    }

    QList<QAction*> actions;
    const QIcon icon = QIcon::fromTheme(QStringLiteral("ark"));

    QMenu *compressMenu = new QMenu(parentWidget);

    compressMenu->addAction(createAction(icon,
                                         i18nc("@action:inmenu Part of Compress submenu in Dolphin context menu", "Here (as TAR.GZ)"),
                                         parentWidget,
                                         fileItemInfos.urlList(),
                                         QStringLiteral("ark --changetofirstpath --add --autofilename tar.gz %F")));

    const QMimeType zipMime = QMimeDatabase().mimeTypeForName(QStringLiteral("application/zip"));
    // Don't offer zip compression if no zip plugin is available.
    if (!m_pluginManager->preferredWritePluginsFor(zipMime).isEmpty()) {
        compressMenu->addAction(createAction(icon,
                                             i18nc("@action:inmenu Part of Compress submenu in Dolphin context menu", "Here (as ZIP)"),
                                             parentWidget,
                                             fileItemInfos.urlList(),
                                             QStringLiteral("ark --changetofirstpath --add --autofilename zip %F")));
    }

    compressMenu->addAction(createAction(icon,
                                         i18nc("@action:inmenu Part of Compress submenu in Dolphin context menu", "Compress to..."),
                                         parentWidget,
                                         fileItemInfos.urlList(),
                                         QStringLiteral("ark --add --changetofirstpath --dialog %F")));

    QAction *compressMenuAction = new QAction(i18nc("@action:inmenu Compress submenu in Dolphin context menu", "Compress"), parentWidget);
    compressMenuAction->setMenu(compressMenu);
    compressMenuAction->setEnabled(fileItemInfos.isLocal() && fileItemInfos.supportsWriting() && !m_pluginManager->availableWritePlugins().isEmpty());

    actions << compressMenuAction;
    return actions;
}

QAction *CompressFileItemAction::createAction(const QIcon& icon, const QString& name, QWidget *parent, const QList<QUrl>& urls, const QString& exec)
{
    QAction *action = new QAction(icon, name, parent);

    connect(action, &QAction::triggered, this, [exec, urls, parent]() {
        KRun::run(exec, urls, parent);
    });

    return action;
}

#include "compressfileitemaction.moc"

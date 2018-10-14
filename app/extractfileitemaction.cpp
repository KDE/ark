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

#include "extractfileitemaction.h"

#include <QFileInfo>
#include <QMenu>

#include <KPluginFactory>
#include <KLocalizedString>
#include <KRun>

#include "mimetypes.h"
#include "pluginmanager.h"

K_PLUGIN_FACTORY_WITH_JSON(ExtractFileItemActionFactory, "extractfileitemaction.json", registerPlugin<ExtractFileItemAction>();)

using namespace Kerfuffle;

ExtractFileItemAction::ExtractFileItemAction(QObject* parent, const QVariantList&)
    : KAbstractFileItemActionPlugin(parent)
    , m_pluginManager(new PluginManager(this))
{}

QList<QAction*> ExtractFileItemAction::actions(const KFileItemListProperties& fileItemInfos, QWidget* parentWidget)
{
    QList<QAction*> actions;
    const QIcon icon = QIcon::fromTheme(QStringLiteral("ark"));

    bool readOnlyParentDir = false;
    QList<QUrl> supportedUrls;
    // Filter URLs by supported mimetypes.
    foreach (const QUrl &url, fileItemInfos.urlList()) {
        const QMimeType mimeType = determineMimeType(url.path());
        if (m_pluginManager->preferredPluginsFor(mimeType).isEmpty()) {
            continue;
        }
        supportedUrls << url;
        // Check whether we can write in the parent directory of the file.
        const QString directory = url.adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash).toLocalFile();
        if (!QFileInfo(directory).isWritable()) {
            readOnlyParentDir = true;
        }
    }

    if (supportedUrls.isEmpty()) {
        return {};
    }

    QMenu *extractMenu = new QMenu(parentWidget);

    extractMenu->addAction(createAction(icon,
                                        i18nc("@action:inmenu Part of Extract submenu in Dolphin context menu", "Extract archive here"),
                                        parentWidget,
                                        supportedUrls,
                                        QStringLiteral("ark --batch --autodestination %F")));

    extractMenu->addAction(createAction(icon,
                                        i18nc("@action:inmenu Part of Extract submenu in Dolphin context menu", "Extract archive to..."),
                                        parentWidget,
                                        supportedUrls,
                                        QStringLiteral("ark --batch --autodestination --dialog %F")));

    extractMenu->addAction(createAction(icon,
                                        i18nc("@action:inmenu Part of Extract submenu in Dolphin context menu", "Extract archive here, autodetect subfolder"),
                                        parentWidget,
                                        supportedUrls,
                                        QStringLiteral("ark --batch --autodestination --autosubfolder %F")));

    QAction *extractMenuAction = new QAction(i18nc("@action:inmenu Extract submenu in Dolphin context menu", "Extract"), parentWidget);
    extractMenuAction->setMenu(extractMenu);

    // #189177: disable extract menu in read-only folders.
    if (readOnlyParentDir) {
        extractMenuAction->setEnabled(false);
    }

    actions << extractMenuAction;
    return actions;
}

QAction *ExtractFileItemAction::createAction(const QIcon& icon, const QString& name, QWidget *parent, const QList<QUrl>& urls, const QString& exec)
{
    QAction *action = new QAction(icon, name, parent);

    connect(action, &QAction::triggered, this, [exec, urls, parent]() {
        KRun::run(exec, urls, parent);
    });

    return action;
}

#include "extractfileitemaction.moc"

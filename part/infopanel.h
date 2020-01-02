/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
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

#ifndef INFOPANEL_H
#define INFOPANEL_H

#include "archivemodel.h"
#include "ui_infopanel.h"

#include <QFrame>

class InfoPanel: public QFrame, Ui::InformationPanel
{
    Q_OBJECT
public:
    explicit InfoPanel(ArchiveModel *model, QWidget *parent = nullptr);
    ~InfoPanel() override;

    void setIndex(const QModelIndex &);
    void setIndexes(const QModelIndexList &list);

    /**
     * Returns the file name that is displayed on the info panel.
     *
     * @return The current file name. If no pretty name has been
     *         set, it returns the name of the loaded archive.
     */
    QString prettyFileName() const;

    /**
     * Sets a different file name for the current open archive.
     *
     * This is particularly useful when a temporary archive (from
     * a remote location) is loaded, and the window title shows the
     * remote file name and the info panel, by default, would show
     * the name of the temporary downloaded file.
     *
     * @param fileName The new file name.
     */
    void setPrettyFileName(const QString& fileName);

    void updateWithDefaults();

private:
    void showMetaData();
    void hideMetaData();

    void showMetaDataFor(const QModelIndex &index);

    QPixmap getPixmap(const QString& name);

    ArchiveModel *m_model;
    QString m_prettyFileName;
};

#endif // INFOPANEL_H

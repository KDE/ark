/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef INFOPANEL_H
#define INFOPANEL_H

#include "archivemodel.h"
#include "ui_infopanel.h"

#include <QFrame>

class InfoPanel : public QFrame, Ui::InformationPanel
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
    void setPrettyFileName(const QString &fileName);

    void updateWithDefaults();

private:
    void showMetaData();
    void hideMetaData();

    void showMetaDataFor(const QModelIndex &index);

    QPixmap getPixmap(const QString &name);

    ArchiveModel *m_model;
    QString m_prettyFileName;
};

#endif // INFOPANEL_H

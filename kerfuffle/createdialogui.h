/*
 * createdialogui.cpp
 *
 * Copyright (C) 2012 basysKom GmbH <info@basyskom.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#ifndef CREATEDIALOGUI_H
#define CREATEDIALOGUI_H

#include <KUrl>
#include <QString>
#include <QMultiHash>

#include "ui_createdialogui.h"
#include "kerfuffle/archive.h"

namespace Kerfuffle
{

class CreateDialogUI: public QWidget, public Ui::CreateDialog
{
    Q_OBJECT

public:
    CreateDialogUI(QWidget *parent = 0);

    CompressionOptions options() const;
    void setOptions(const CompressionOptions& options = CompressionOptions());

    KUrl archiveUrl() const;
    void setArchiveUrl(const KUrl& archiveUrl);

public slots:
    bool checkArchiveUrl();

private slots:
    void browse();
    void updateArchiveExtension(bool updateCombobox = false);
    void updateUi();

private:
    QMultiHash<QString,int> m_mimeTypeOptions;

};
}

#endif // CREATEDIALOGUI_H

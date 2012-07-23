/*
 * archiveconflictdialog.cpp
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

#include "archiveconflictdialog.h"

#include <QtCore/QString>
#include <QtGui/QHBoxLayout>
#include <QtGui/QIcon>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QMessageBox>
#include <QtGui/QRadioButton>
#include <QtGui/QStyleOption>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

#include <KIconLoader>
#include <KLocalizedString>


ArchiveConflictDialog::ArchiveConflictDialog(QWidget * parent, QString archive, QString suggestedName)
    : KDialog(parent),
      m_overwriteButton(0),
      m_saveAsButton(0),
      m_openExistingButton(0)
{
    QWidget *mainWidget = new QWidget(this);
    setMainWidget(mainWidget);

    QHBoxLayout *contentLayout = new QHBoxLayout(mainWidget);
    contentLayout->setMargin(KDialog::marginHint());
    contentLayout->setSpacing(KDialog::spacingHint());
    mainWidget->setLayout(contentLayout);

    QLabel *iconLabel = new QLabel();
    QIcon icon = KIconLoader::global()->loadIcon(QLatin1String("dialog-warning"), KIconLoader::NoGroup,
                 KIconLoader::SizeEnormous, KIconLoader::DefaultState,
                 QStringList(), 0, true);
    QStyleOption option;
    option.initFrom(mainWidget);
    iconLabel->setPixmap(icon.pixmap(mainWidget->style()->pixelMetric(QStyle::PM_MessageBoxIconSize, &option, mainWidget)));

    QVBoxLayout *iconLayout = new QVBoxLayout();
    iconLayout->addStretch(1);
    iconLayout->addWidget(iconLabel);
    iconLayout->addStretch(5);

    contentLayout->addLayout(iconLayout);

    QVBoxLayout *optionsLayout = new QVBoxLayout();
    optionsLayout->setSpacing(KDialog::spacingHint());
    optionsLayout->setMargin(KDialog::marginHint());

    QLabel *text = new QLabel(i18nc("@info", "The archive <filename>%1</filename> already exists.<nl/>Please select one of the following options:", archive));
    optionsLayout->addWidget(text);

    QVBoxLayout *radioButtonsLayout = new QVBoxLayout();
    radioButtonsLayout->setSpacing(-1);
    radioButtonsLayout->setMargin(0);

    m_overwriteButton = new QRadioButton(i18nc("@option:radio", "Overwrite existing archive"));
    m_saveAsButton = new QRadioButton(i18nc("@option:radio", "Save new archive as <filename>%1</filename>", suggestedName));
    m_saveAsButton->setChecked(true);
    m_openExistingButton = new QRadioButton(i18nc("@option:radio", "Add files to existing Archive"));

    radioButtonsLayout->addWidget(m_overwriteButton);
    radioButtonsLayout->addWidget(m_saveAsButton);
    radioButtonsLayout->addWidget(m_openExistingButton);

    optionsLayout->addLayout(radioButtonsLayout);
    contentLayout->addLayout(optionsLayout);

    setCaption(i18nc("@title:window", "Archive Already Exists"));
    setButtons(KDialog::Ok | KDialog::Cancel);
    setDefaultButton(KDialog::Ok);
    setEscapeButton(KDialog::Cancel);
}

int ArchiveConflictDialog::selectedOption()
{
    if (m_overwriteButton->isChecked()) {
        return OverwriteExisting;
    } else if (m_saveAsButton->isChecked()) {
        return RenameNew;
    } else {
        return OpenExisting;
    }
}

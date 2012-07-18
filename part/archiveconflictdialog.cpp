/*
 * Copyright (C) 2012 basysKom GmbH <info@basyskom.com>
 */

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

#include "archiveconflictdialog.h"


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
    QIcon icon = KIconLoader::global()->loadIcon("dialog-warning", KIconLoader::NoGroup,
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

    QLabel *text = new QLabel(i18nc("@info", "The archive <filename>%1</filename> already exists.<br>Please select one of the following options:", archive));
    optionsLayout->addWidget(text);

    QVBoxLayout *radioButtonsLayout = new QVBoxLayout();
    radioButtonsLayout->setSpacing(-1);
    radioButtonsLayout->setMargin(0);

    m_overwriteButton = new QRadioButton(i18n("Overwrite existing archive"));
    m_saveAsButton = new QRadioButton(i18n("Save new arvchive as <filename>%1</filename>", suggestedName));
    m_saveAsButton->setChecked(true);
    m_openExistingButton = new QRadioButton(i18n("Add files to existing Archive"));

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

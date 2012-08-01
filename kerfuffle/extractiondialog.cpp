/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2009 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2012 basyKom GmbH <info@basyskom.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// TODO: create custom directory selector widget with a linedit and places selector similar
// to a KUrlNavigator on top, that shows the selected path/path to create, and a directory tree
// view below that does not enter empty directories and just shows the selected directory
// The existing KDirSelectorDialog is too big to be used here (and we don't want all the navigator
// stuff)

#include "extractiondialog.h"
#include "ui_extractiondialog.h"
#include "archive.h"

#include <QtGui/QAbstractItemView>
#include <QtCore/QDir>

#include <KConfigGroup>
#include <KDebug>
#include <KDirOperator>
#include <KFilePlacesModel>
#include <KGlobal>
#include <KGuiItem>
#include <KIO/NetAccess>
#include <KTabWidget>
#include <KLocale>
#include <KMessageBox>
#include <KStandardDirs>
#include <KUrlNavigator>

namespace Kerfuffle
{

class ExtractionDialogUI: public QWidget, public Ui::ExtractionDialog
{
public:
    ExtractionDialogUI(QWidget *parent = 0)
        : QWidget(parent) {
        setupUi(this);

        urlNavigator = new KUrlNavigator(new KFilePlacesModel(this), KUrl(QDir::homePath()), 0);
        urlNavigator->setPlacesSelectorVisible(true);
        destinationTab->layout()->addWidget(urlNavigator);
        dirOperator = new KDirOperator(KUrl(QDir::homePath()));
        dirOperator->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        dirOperator->setMode(KFile::Directory | KFile::File);
        dirOperator->setView(KFile::Tree);
        dirOperator->view()->setSelectionMode(QAbstractItemView::SingleSelection);
        dirOperator->setNewFileMenuSupportedMimeTypes(QStringList() << QLatin1String("inode/directory"));
        dirOperator->dirLister()->setAutoUpdate(true);
        destinationTab->layout()->addWidget(dirOperator);
    }

    KDirOperator *dirOperator;
    KUrlNavigator *urlNavigator;
};

ExtractionDialog::ExtractionDialog(QWidget *parent)
    : KDialog(parent),
      m_url(KUrl(QDir::homePath()))
{
    m_ui = new ExtractionDialogUI(this);

    setMainWidget(m_ui);
    setButtons(KDialog::User1 | KDialog::Reset |  KDialog::Ok | KDialog::Cancel);
    setButtonGuiItem(KDialog::User1,
                     KGuiItem(i18nc("@action:button", "Set As Default"),
                              QLatin1String("configure"),
                              i18nc("@info:tooltip", "Set the selected values as default values"),
                              i18nc("@info:whatsthis", "Sets the selected values as default values for new archives")));

    setBatchMode(false);

    m_config = KConfigGroup(KGlobal::config()->group("ExtractionDialog"));
    loadSettings();

    connect(this, SIGNAL(resetClicked()), SLOT(loadSettings()));
    connect(this, SIGNAL(user1Clicked()), SLOT(writeSettings()));
    connect(m_ui->dirOperator, SIGNAL(urlEntered(KUrl)), this, SLOT(setDestination(KUrl)));
    connect(m_ui->urlNavigator, SIGNAL(urlChanged(KUrl)), this, SLOT(setDestination(KUrl)));
}

ExtractionDialog::~ExtractionDialog()
{
    delete m_ui;
    m_ui = 0;
}

void ExtractionDialog::loadSettings()
{
    kDebug();
    ExtractionOptions options;
    foreach(const QString & str, m_config.keyList()) {
        options[str] = m_config.readEntry(str);
    }

    // set options in the UI
    setOptions(options);
}

void ExtractionDialog::writeSettings()
{
    kDebug();
    // get options from the UI
    QHashIterator<QString, QVariant> it((QHash<QString, QVariant>)options());
    while (it.hasNext()) {
        it.next();
        m_config.writeEntry(it.key(), it.value());
    }
}

void ExtractionDialog::updateView()
{
    kDebug();
    disconnect(m_ui->dirOperator, SIGNAL(urlEntered(KUrl)), this, SLOT(setDestination(KUrl)));
    disconnect(m_ui->urlNavigator, SIGNAL(urlChanged(KUrl)), this, SLOT(setDestination(KUrl)));

    m_ui->dirOperator->setUrl(m_url, true);
    m_ui->dirOperator->dirLister()->updateDirectory(m_ui->dirOperator->url());
    m_ui->urlNavigator->setLocationUrl(m_url);

    connect(m_ui->dirOperator, SIGNAL(urlEntered(KUrl)), SLOT(setDestination(KUrl)));
    connect(m_ui->urlNavigator, SIGNAL(urlChanged(KUrl)), SLOT(setDestination(KUrl)));
}

void ExtractionDialog::setBatchMode(bool enabled)
{
    kDebug();
    if (enabled) {
        m_ui->autoSubfoldersCheckBox->show();
        m_ui->autoSubfoldersCheckBox->setChecked(true);
        setCaption(i18nc("@title:window", "Extract multiple archives"));
    } else {
        m_ui->autoSubfoldersCheckBox->hide();
        setCaption(i18nc("@title:window", "Extract archive"));
    }
}

void ExtractionDialog::accept()
{
    kDebug();
    const QString path = m_url.pathOrUrl(KUrl::AddTrailingSlash);

    if (!KIO::NetAccess::exists(path, KIO::NetAccess::SourceSide, 0)
            && !KIO::NetAccess::mkdir(path, 0)) {
        KMessageBox::detailedError(0,
                                   i18nc("@info", "The folder <filename>%1</filename> could not be created.", path),
                                   i18nc("@info", "Please check your permissions to create it."));
        return;
    }

    KDialog::accept();
}

void ExtractionDialog::setOptions(const ExtractionOptions &options)
{
    kDebug();
    m_ui->autoSubfoldersCheckBox->setChecked(options.value(QLatin1String("AutoSubfolders"),
            m_config.readEntry("AutoSubfolders", false)).toBool());

    m_ui->openFolderCheckBox->setChecked(options.value(QLatin1String("OpenDestinationAfterExtraction"),
            m_config.readEntry("OpenDestinationAfterExtraction", false)).toBool());

    m_ui->closeAfterExtractionCheckBox->setChecked(options.value(QLatin1String("CloseArkAfterExtraction"),
            m_config.readEntry("CloseArkAfterExtraction", false)).toBool());

    m_ui->utf8CheckBox->setChecked(options.value(QLatin1String("FixFileNameEncoding"),
             m_config.readEntry("FixFileNameEncoding", true)).toBool());

    m_ui->multithreadingCheckBox->setChecked(options.value(QLatin1String("MultiThreadingEnabled"),
            m_config.readEntry("MultiThreadingEnabled", true)).toBool());

    m_ui->preservePathsCheckBox->setChecked(options.value(QLatin1String("PreservePaths"),
            m_config.readEntry("PreservePaths", false)).toBool());

    setDestination(KUrl(options.value(QLatin1String("DestinationDirectory"),
            m_config.readEntry("DestinationDirectory", QDir::homePath())).toString()));
}

ExtractionOptions ExtractionDialog::options() const
{
    kDebug();
    ExtractionOptions options;
    options[QLatin1String("AutoSubfolders")] = m_ui->autoSubfoldersCheckBox->isChecked();
    options[QLatin1String("OpenDestinationAfterExtraction")] = m_ui->openFolderCheckBox->isChecked();
    options[QLatin1String("CloseArkAfterExtraction")] = m_ui->closeAfterExtractionCheckBox->isChecked();
    options[QLatin1String("FixFileNameEncoding")] = m_ui->utf8CheckBox->isChecked();
    options[QLatin1String("MultiThreadingEnabled")] = m_ui->multithreadingCheckBox->isChecked();
    options[QLatin1String("PreservePaths")] = m_ui->preservePathsCheckBox->isChecked();
    options[QLatin1String("DestinationDirectory")] = destination();

    return options;
}

KUrl ExtractionDialog::destination() const
{
    return m_url;
}

void ExtractionDialog::setDestination(const KUrl &url)
{
    kDebug();
    m_url = url;
    updateView();
}
}

#include "extractiondialog.moc"

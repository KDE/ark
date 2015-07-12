/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009,2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
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

#include "app/logging.h"
#include "adddialog.h"
#include "ui_adddialog.h"
#include "kerfuffle/archive_kerfuffle.h"


#include <KFilePlacesModel>
#include <KSharedConfig>
#include <KWindowConfig>
#include <KUrlComboBox>
#include <KMessageBox>

#include <QFileInfo>
#include <QStandardItemModel>
#include <QPushButton>
#include <QIcon>
#include <QMimeDatabase>
#include <QDebug>
#include <QWindow>
#include <QScreen>

namespace Kerfuffle
{
class AddDialogUI: public QWidget, public Ui::AddDialog
{
public:
    AddDialogUI(QWidget *parent = 0)
            : QWidget(parent) {
        setupUi(this);
    }
};

AddDialog::AddDialog(const QStringList &itemsToAdd,
                     QWidget *parent,
                     const QString &caption,
                     const QUrl &startDir)
        : QDialog(parent, Qt::Dialog)
{
    qCDebug(KERFUFFLE) << "AddDialog loaded";

    this->setWindowTitle(caption);

    QHBoxLayout *hlayout = new QHBoxLayout();
    setLayout(hlayout);

    fileWidget = new KFileWidget(startDir, this);
    hlayout->addWidget(fileWidget);

    fileWidget->setMode(KFile::File | KFile::LocalOnly);
    fileWidget->setConfirmOverwrite(true);
    fileWidget->setOperationMode(KFileWidget::Saving);

    connect(fileWidget->okButton(), &QPushButton::clicked, this, &AddDialog::slotOkButtonClicked);
    connect(fileWidget, &KFileWidget::accepted, fileWidget, &KFileWidget::accept);
    connect(fileWidget, &KFileWidget::accepted, this, &QDialog::accept);
    fileWidget->okButton()->show();

    fileWidget->cancelButton()->show();
    connect(fileWidget->cancelButton(), &QPushButton::clicked, this, &QDialog::reject);

    loadConfiguration();

    connect(this, &QDialog::accepted, this, &AddDialog::updateDefaultMimeType);
    connect(this, &QDialog::finished, this, &AddDialog::slotSaveWindowSize);

    // Sidepanel with extra options, disabled for now
    /*
    m_ui = new AddDialogUI(this);
    hlayout->addWidget(m_ui);
    m_ui->groupExtraOptions->hide();
    setupIconList(itemsToAdd);
    */

    // Set up a default name if there's only one file to compress
    if (itemsToAdd.size() == 1) {
        const QFileInfo fileInfo(itemsToAdd.first());
        const QString fileName =
            fileInfo.isDir() ? fileInfo.dir().dirName() : fileInfo.baseName();
        fileWidget->setSelection(fileName);
    }
}

QSize AddDialog::sizeHint() const
{
    // Used only when no previous window size has been stored
    return QSize(750,450);
}

void AddDialog::loadConfiguration()
{
    m_config = KConfigGroup(KSharedConfig::openConfig()->group("AddDialog"));

    const QString defaultMimeType = QLatin1String("application/x-compressed-tar");
    const QString lastMimeType = m_config.readEntry("LastMimeType", defaultMimeType);
    QStringList writeMimeTypes = Kerfuffle::supportedWriteMimeTypes();

    // The filters need to be sorted by comment, so create a QMap with
    // comment as key (QMaps are always sorted by key) and QMimeType
    // as value. Then convert the QMap back to a QStringList. Mimetypes
    // with empty comments are discarded.
    QMimeDatabase db;
    QMap<QString,QMimeType> mimeMap;
    foreach (const QString &s, writeMimeTypes) {
        QMimeType mime(db.mimeTypeForName(s));
        if (!mime.comment().isEmpty()) {
            mimeMap[mime.comment()] = mime;
        }
    }

    writeMimeTypes.clear();

    QMapIterator<QString,QMimeType> j(mimeMap);
    while (j.hasNext()) {
        j.next();
        writeMimeTypes << j.value().name();
    }

    if (writeMimeTypes.contains(lastMimeType)) {
        fileWidget->setMimeFilter(writeMimeTypes, lastMimeType);
    } else {
        fileWidget->setMimeFilter(writeMimeTypes, defaultMimeType);
    }
}

void AddDialog::setupIconList(const QStringList& itemsToAdd)
{
    QStandardItemModel* listModel = new QStandardItemModel(this);
    QStringList sortedList(itemsToAdd);

    sortedList.sort();

    Q_FOREACH(const QString& urlString, sortedList) {
        QUrl url(urlString);

        QStandardItem* item = new QStandardItem;
        item->setText(url.fileName());

        QMimeDatabase db;
        QString iconName = db.mimeTypeForUrl(url).iconName();
        item->setIcon(QIcon::fromTheme(iconName));

        item->setData(QVariant(url), KFilePlacesModel::UrlRole);

        listModel->appendRow(item);
    }

    m_ui->compressList->setModel(listModel);
}

void AddDialog::updateDefaultMimeType()
{
    m_config.writeEntry("LastMimeType", fileWidget->currentFilterMimeType().name());
}

QList<QUrl> AddDialog::selectedUrls()
{
    return(fileWidget->selectedUrls());
}

QString AddDialog::currentMimeFilter()
{
    return(fileWidget->currentMimeFilter());
}

void AddDialog::slotSaveWindowSize()
{
    // Save dialog window size
    KConfigGroup group(KSharedConfig::openConfig(), "AddDialog");
    KWindowConfig::saveWindowSize(windowHandle(), group, KConfigBase::Persistent);
}

void AddDialog::slotOkButtonClicked()
{
    // In case the user tries to leave the lineEdit empty:
    if (fileWidget->locationEdit()->urls().at(0) == fileWidget->baseUrl().path().left(fileWidget->baseUrl().path().size()-1))
    {
        KMessageBox::sorry(this, i18n("Please select a filename for the archive."), i18n("No file selected"));
        return;
    }
    // This slot sets the url from text in the lineEdit, asks for overwrite etc, and emits signal accepted
    fileWidget->slotOk();
}

void AddDialog::restoreWindowSize()
{
  // Restore window size from config file, needs a windowHandle so must be called after show()
  KConfigGroup group(KSharedConfig::openConfig(), "AddDialog");
  //KWindowConfig::restoreWindowSize(windowHandle(), group);
  //KWindowConfig::restoreWindowSize is broken atm., so we need this hack:
  const QRect desk = windowHandle()->screen()->geometry();
  this->resize(QSize(group.readEntry(QString::fromLatin1("Width %1").arg(desk.width()), windowHandle()->size().width()),
                     group.readEntry(QString::fromLatin1("Height %1").arg(desk.height()), windowHandle()->size().height())));
}
}

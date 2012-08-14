/* This file is part of the KDE libraries
    Copyright (C) 2000 Stephan Kulow <coolo@kde.org>
                  1999 - 2008 David Faure <faure@kde.org>
                  2001, 2006 Holger Freyther <freyther@kde.org>
    Copyright (C) 2012 basysKom GmbH <info@basyskom.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "renamedialog.h"
#include <stdio.h>
#include <assert.h>

#include <QtCore/QDate>
#include <QtCore/QFileInfo>
#include <QtGui/QCheckBox>
#include <QtGui/QLabel>
#include <QtGui/QLayout>
#include <QtGui/QPixmap>
#include <QtGui/QScrollArea>
#include <QtGui/QScrollBar>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtCore/QDir>

#include <KLineEdit>
#include <KMessageBox>
#include <KPushButton>
#include <kio/udsentry.h>
#include <KDialog>
#include <KLocale>
#include <KGlobal>
#include <KDebug>
#include <KFileItem>
#include <KMimeType>
#include <KSeparator>
#include <KStringHandler>
#include <KStandardGuiItem>
#include <KGuiItem>
#include <KFileMetaDataWidget>
#include <KIO/PreviewJob>


namespace Kerfuffle
{

/** @internal */
class RenameDialog::RenameDialogPrivate
{
public:
    RenameDialogPrivate() {
        bCancel = 0;
        bRename = bSkip = bOverwrite = 0;
        bResume = bSuggestNewName = 0;
        bApplyAll = 0;
        bUpdate = 0;
        m_pLineEdit = 0;
        m_srcPendingPreview = false;
        m_destPendingPreview = false;
        m_srcPreview = 0;
        m_destPreview = 0;
    }

    void setRenameBoxText(const QString& fileName) {
        // sets the text in file name line edit box, selecting the filename (but not the extension if there is one).
        const QString extension = KMimeType::extractKnownExtension(fileName);
        m_pLineEdit->setText(fileName);

        if (!extension.isEmpty()) {
            const int selectionLength = fileName.length() - extension.length() - 1;
            m_pLineEdit->setSelection(0, selectionLength);
        } else {
            m_pLineEdit->selectAll();
        }
    }

    KPushButton *bCancel;
    QPushButton *bRename;
    QPushButton *bSkip;
    QPushButton *bOverwrite;
    QPushButton *bResume;
    QPushButton *bSuggestNewName;
    QPushButton *bUpdate;
    QCheckBox *bApplyAll;
    KLineEdit* m_pLineEdit;
    KUrl src;
    KUrl dest;
    bool m_srcPendingPreview;
    bool m_destPendingPreview;
    QLabel* m_srcPreview;
    QLabel* m_destPreview;
    QScrollArea* m_srcArea;
    QScrollArea* m_destArea;
    KFileItem srcItem;
    KFileItem destItem;
};

RenameDialog::RenameDialog(QWidget *parent, const QString & _caption,
                           const KUrl &_src, const KUrl &_dest,
                           RenameDialog_Mode _mode,
                           KIO::filesize_t sizeSrc,
                           KIO::filesize_t sizeDest,
                           time_t ctimeSrc,
                           time_t ctimeDest,
                           time_t mtimeSrc,
                           time_t mtimeDest)
    : QDialog(parent), d(new RenameDialogPrivate)
{
    setObjectName(QLatin1String("Kerfuffle::RenameDialog"));

    d->src = _src;
    d->dest = _dest;

    setWindowTitle(_caption);

    d->bCancel = new KPushButton(KStandardGuiItem::cancel(), this);
    connect(d->bCancel, SIGNAL(clicked()), this, SLOT(cancelPressed()));

    if (_mode & M_UPDATE_EXISTING) {
        _mode = (RenameDialog_Mode)(_mode | M_MULTI); // M_MULTI is implied by setting M_UPDATE_EXISTING
    }

    if (_mode & M_MULTI) {
        d->bApplyAll = new QCheckBox(i18n("Appl&y to All"), this);
        d->bApplyAll->setToolTip((_mode & M_ISDIR) ? i18n("When this is checked the button pressed will be applied to all subsequent folder conflicts for the remainder of the current job.\nUnless you press Skip you will still be prompted in case of a conflict with an existing file in the directory.")
                                 : i18n("When this is checked the button pressed will be applied to all subsequent conflicts for the remainder of the current job."));
        connect(d->bApplyAll, SIGNAL(clicked()), this, SLOT(applyAllPressed()));
    }

    if (!(_mode & M_NORENAME)) {
        d->bRename = new QPushButton(i18n("&Rename"), this);
        d->bRename->setEnabled(false);
        if (!(_mode & M_AUTO_RENAME)) {
            d->bSuggestNewName = new QPushButton(i18n("Suggest New &Name"), this);
            connect(d->bSuggestNewName, SIGNAL(clicked()), this, SLOT(suggestNewNamePressed()));
        }
        connect(d->bRename, SIGNAL(clicked()), this, SLOT(renamePressed()));
    }

    if ((_mode & M_MULTI) && (_mode & M_SKIP)) {
        d->bSkip = new QPushButton(i18n("&Skip"), this);
        d->bSkip->setToolTip((_mode & M_ISDIR) ? i18n("Do not copy or move this folder, skip to the next item instead")
                             : i18n("Do not copy or move this file, skip to the next item instead"));
        connect(d->bSkip, SIGNAL(clicked()), this, SLOT(skipPressed()));
    }

    if (_mode & M_OVERWRITE) {
        const QString text = (_mode & M_ISDIR) ? i18nc("Write files into an existing folder", "&Write Into") : i18n("&Overwrite");
        d->bOverwrite = new QPushButton(text, this);
        d->bOverwrite->setToolTip(i18n("Files and folders will be copied into the existing directory, alongside its existing contents.\nYou will be prompted again in case of a conflict with an existing file in the directory."));
        connect(d->bOverwrite, SIGNAL(clicked()), this, SLOT(overwritePressed()));
    }

    if (_mode & M_RESUME) {
        d->bResume = new QPushButton(i18n("&Resume"), this);
        connect(d->bResume, SIGNAL(clicked()), this, SLOT(resumePressed()));
    }

    if ((_mode & M_MULTI) && (_mode & M_UPDATE_EXISTING)) {
        d->bUpdate = new QPushButton(i18n("Refresh &Existing Only"), this);
        if (d->bApplyAll) {
            d->bUpdate->setEnabled(d->bApplyAll->isChecked());
        }
        d->bUpdate->setToolTip(i18n("Overwrites only existing files but does not add new files"));
        connect(d->bUpdate, SIGNAL(clicked()), this, SLOT(updateExistingPressed()));
    }

    QVBoxLayout* pLayout = new QVBoxLayout(this);
    pLayout->addStrut(400);     // makes dlg at least that wide

    // User tries to overwrite a file with itself ?
    if (_mode & M_OVERWRITE_ITSELF) {
        QLabel *lb = new QLabel(i18n("This action would overwrite '%1' with itself.\n"
                                     "Please enter a new file name:",
                                     KStringHandler::csqueeze(d->src.pathOrUrl(), 100)), this);

        d->bRename->setText(i18n("C&ontinue"));
        pLayout->addWidget(lb);
    } else if (_mode & M_OVERWRITE) {
        if (d->src.isLocalFile()) {
            d->srcItem = KFileItem(KFileItem::Unknown, KFileItem::Unknown, d->src);
        } else {
            KIO::UDSEntry srcUds;

            srcUds.insert(KIO::UDSEntry::UDS_NAME, d->src.fileName());
            srcUds.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME, mtimeSrc);
            srcUds.insert(KIO::UDSEntry::UDS_CREATION_TIME, ctimeSrc);
            srcUds.insert(KIO::UDSEntry::UDS_SIZE, sizeSrc);

            d->srcItem = KFileItem(srcUds, d->src);
        }

        if (d->dest.isLocalFile()) {
            d->destItem = KFileItem(KFileItem::Unknown, KFileItem::Unknown, d->dest);
        } else {
            KIO::UDSEntry destUds;

            destUds.insert(KIO::UDSEntry::UDS_NAME, d->dest.fileName());
            destUds.insert(KIO::UDSEntry::UDS_MODIFICATION_TIME, mtimeDest);
            destUds.insert(KIO::UDSEntry::UDS_CREATION_TIME, ctimeDest);
            destUds.insert(KIO::UDSEntry::UDS_SIZE, sizeDest);

            d->destItem = KFileItem(destUds, d->dest);
        }

        d->m_srcPreview = createLabel(parent, QString(), false);
        d->m_destPreview = createLabel(parent, QString(), false);

        d->m_srcPreview->setMinimumHeight(KIconLoader::SizeEnormous);
        d->m_destPreview->setMinimumHeight(KIconLoader::SizeEnormous);

        d->m_srcPreview->setAlignment(Qt::AlignCenter);
        d->m_destPreview->setAlignment(Qt::AlignCenter);

        d->m_srcPendingPreview = true;
        d->m_destPendingPreview = true;

        // widget
        d->m_srcArea = createContainerLayout(parent, d->srcItem, d->m_srcPreview);
        d->m_destArea = createContainerLayout(parent, d->destItem, d->m_destPreview);

        connect(d->m_srcArea->verticalScrollBar(), SIGNAL(valueChanged(int)), d->m_destArea->verticalScrollBar(), SLOT(setValue(int)));
        connect(d->m_destArea->verticalScrollBar(), SIGNAL(valueChanged(int)), d->m_srcArea->verticalScrollBar(), SLOT(setValue(int)));
        connect(d->m_srcArea->horizontalScrollBar(), SIGNAL(valueChanged(int)), d->m_destArea->horizontalScrollBar(), SLOT(setValue(int)));
        connect(d->m_destArea->horizontalScrollBar(), SIGNAL(valueChanged(int)), d->m_srcArea->horizontalScrollBar(), SLOT(setValue(int)));

        // create layout
        QGridLayout* gridLayout = new QGridLayout();
        pLayout->addLayout(gridLayout);

        QLabel* titleLabel = new QLabel(i18n("This action will overwrite the destination."), this);

        QLabel* srcTitle = createLabel(parent, i18nc("The source file or directory", "Source"), true);
        QLabel* destTitle = createLabel(parent, i18nc("The destination file or directory", "Destination"), true);

        QLabel* srcInfo = createSqueezedLabel(parent, d->src.pathOrUrl());
        QLabel* destInfo = createSqueezedLabel(parent, d->dest.pathOrUrl());

        if (mtimeDest > mtimeSrc) {
            QLabel* warningLabel = new QLabel(i18n("Warning, the destination is more recent."), this);

            gridLayout->addWidget(titleLabel, 0, 0, 1, 2);    // takes the complete first line
            gridLayout->addWidget(warningLabel, 1, 0, 1, 2);
            gridLayout->setRowMinimumHeight(2, 15);    // spacer

            gridLayout->addWidget(srcTitle, 3, 0);
            gridLayout->addWidget(srcInfo, 4, 0);
            gridLayout->addWidget(d->m_srcArea, 5, 0);

            gridLayout->addWidget(destTitle, 3, 1);
            gridLayout->addWidget(destInfo, 4, 1);
            gridLayout->addWidget(d->m_destArea, 5, 1);
        } else {
            gridLayout->addWidget(titleLabel, 0, 0, 1, 2);
            gridLayout->setRowMinimumHeight(1, 15);

            gridLayout->addWidget(srcTitle, 2, 0);
            gridLayout->addWidget(srcInfo, 3, 0);
            gridLayout->addWidget(d->m_srcArea, 4, 0);

            gridLayout->addWidget(destTitle, 2, 1);
            gridLayout->addWidget(destInfo, 3, 1);
            gridLayout->addWidget(d->m_destArea, 4, 1);
        }
    } else {
        // This is the case where we don't want to allow overwriting, the existing
        // file must be preserved (e.g. when renaming).
        QString sentence1;

        if (mtimeDest < mtimeSrc)
            sentence1 = i18n("An older item named '%1' already exists.", d->dest.pathOrUrl());
        else if (mtimeDest == mtimeSrc)
            sentence1 = i18n("A similar file named '%1' already exists.", d->dest.pathOrUrl());
        else
            sentence1 = i18n("A more recent item named '%1' already exists.", d->dest.pathOrUrl());

        QLabel *lb = new KSqueezedTextLabel(sentence1, this);
        pLayout->addWidget(lb);
    }

    if ((_mode != M_OVERWRITE_ITSELF) && (!(_mode & M_NORENAME))) {
        if (_mode == M_OVERWRITE && !(_mode & M_AUTO_RENAME)) {
            pLayout->addSpacing(15);    // spacer
        }

        if (d->bRename && (_mode & M_AUTO_RENAME)) {
            d->bRename->setEnabled(true);
            QLabel *lb2 = new QLabel(i18nc("@label:textbox", "Rename to <filename>%1</filename>?", autoDestUrl().fileName()), this);
            pLayout->addWidget(lb2);
        } else {
            QLabel *lb2 = new QLabel(i18nc("@label:textbox", "Rename to:"), this);
            pLayout->addWidget(lb2);
        }
    }

    QHBoxLayout* layout2 = new QHBoxLayout();
    pLayout->addLayout(layout2);

    if (!(_mode & M_AUTO_RENAME)) {
        d->m_pLineEdit = new KLineEdit(this);
        layout2->addWidget(d->m_pLineEdit);
    }

    if (d->bRename && d->m_pLineEdit) {
        const QString fileName = d->dest.fileName();
        d->setRenameBoxText(KIO::decodeFileName(fileName));

        connect(d->m_pLineEdit, SIGNAL(textChanged(QString)),
                SLOT(enableRenameButton(QString)));

        d->m_pLineEdit->setFocus();
    } else if (d->m_pLineEdit) {
        d->m_pLineEdit->hide();
    }

    if (d->bSuggestNewName) {
        layout2->addWidget(d->bSuggestNewName);
        setTabOrder(d->m_pLineEdit, d->bSuggestNewName);
    }

    KSeparator* separator = new KSeparator(this);
    pLayout->addWidget(separator);

    QHBoxLayout* layout = new QHBoxLayout();
    pLayout->addLayout(layout);

    layout->addStretch(1);

    if (d->bApplyAll) {
        layout->addWidget(d->bApplyAll);
        setTabOrder(d->bApplyAll, d->bCancel);
    }

    if (d->bRename) {
        layout->addWidget(d->bRename);
        setTabOrder(d->bRename, d->bCancel);
    }

    if (d->bSkip) {
        layout->addWidget(d->bSkip);
        setTabOrder(d->bSkip, d->bCancel);
    }

    if (d->bOverwrite) {
        layout->addWidget(d->bOverwrite);
        setTabOrder(d->bOverwrite, d->bCancel);
    }

    if (d->bResume) {
        layout->addWidget(d->bResume);
        setTabOrder(d->bResume, d->bCancel);
    }
    if (d->bUpdate) {
        layout->addWidget(d->bUpdate);
        setTabOrder(d->bUpdate, d->bCancel);
    }

    d->bCancel->setDefault(true);
    layout->addWidget(d->bCancel);

    resize(sizeHint());
}

RenameDialog::~RenameDialog()
{
    delete d;
    // no need to delete Pushbuttons,... qt will do this
}

void RenameDialog::enableRenameButton(const QString &newDest)
{
    if (newDest != KIO::decodeFileName(d->dest.fileName()) && !newDest.isEmpty()) {
        d->bRename->setEnabled(true);
        d->bRename->setDefault(true);

        if (d->bOverwrite) {
            d->bOverwrite->setEnabled(false);   // prevent confusion (#83114)
        }
    } else {
        d->bRename->setEnabled(false);

        if (d->bOverwrite) {
            d->bOverwrite->setEnabled(true);
        }
    }
}

KUrl RenameDialog::newDestUrl()
{
    if (d->m_pLineEdit && !d->m_pLineEdit->isHidden() && !d->m_pLineEdit->text().isEmpty()) {
        KUrl newDest(d->dest);
        QString fileName = d->m_pLineEdit->text();

        newDest.setFileName(KIO::encodeFileName(fileName));

        return newDest;
    } else {
        return autoDestUrl();
    }
}

KUrl RenameDialog::autoDestUrl() const
{
    KUrl newDest(d->dest);
    KUrl destDirectory(d->dest);

    destDirectory.setPath(destDirectory.directory());
    newDest.setFileName(suggestName(destDirectory, d->dest.fileName()));

    return newDest;
}

void RenameDialog::cancelPressed()
{
    done(R_CANCEL);
}

// Rename
void RenameDialog::renamePressed()
{
    if (d->bApplyAll  && d->bApplyAll->isChecked()) {
        done(R_AUTO_RENAME);
    } else {
        KUrl u = newDestUrl();

        if (!u.isValid()) {
            KMessageBox::error(this, i18n("Malformed URL\n%1" ,  u.url()));
            return;
        }

        done(R_RENAME);
    }
}

QString RenameDialog::suggestName(const KUrl& baseURL, const QString& oldName)
{
    QString dotSuffix, suggestedName;
    QString basename = oldName;
    const QLatin1Char spacer(' ');

    //ignore dots at the beginning, that way "..aFile.tar.gz" will become "..aFile 1.tar.gz" instead of " 1..aFile.tar.gz"
    int index = basename.indexOf(QLatin1Char('.'));
    int continuous = 0;
    while (continuous == index) {
        index = basename.indexOf(QLatin1Char('.'), index + 1);
        ++continuous;
    }

    if (index != -1) {
        dotSuffix = basename.mid(index);
        basename.truncate(index);
    }

    int pos = basename.lastIndexOf(spacer);

    if (pos != -1) {
        QString tmp = basename.mid(pos + 1);
        bool ok;
        int number = tmp.toInt(&ok);

        if (!ok) {  // ok there is no number
            suggestedName = basename + spacer + QLatin1Char('1') + dotSuffix;
        } else {
            // yes there's already a number behind the spacer so increment it by one
            basename.replace(pos + 1, tmp.length(), QString::number(number + 1));
            suggestedName = basename + dotSuffix;
        }
    } else // no spacer yet
        suggestedName = basename + spacer + QLatin1Char('1') + dotSuffix ;

    // Check if suggested name already exists
    bool exists = false;
    // TODO: network transparency. However, using NetAccess from a modal dialog
    // could be a problem, no? (given that it uses a modal widget itself....)
    if (baseURL.isLocalFile())
        exists = QFileInfo(baseURL.toLocalFile(KUrl::AddTrailingSlash) + suggestedName).exists();

    if (!exists)
        return suggestedName;
    else // already exists -> recurse
        return suggestName(baseURL, suggestedName);
}

// Propose button clicked
void RenameDialog::suggestNewNamePressed()
{
    /* no name to play with */
    if (d->m_pLineEdit->text().isEmpty())
        return;

    KUrl destDirectory(d->dest);

    destDirectory.setPath(destDirectory.directory());
    d->setRenameBoxText(suggestName(destDirectory, d->m_pLineEdit->text()));

    return;
}

void RenameDialog::skipPressed()
{
    if (d->bApplyAll  && d->bApplyAll->isChecked()) {
        done(R_AUTO_SKIP);
    } else {
        done(R_SKIP);
    }
}

void RenameDialog::autoSkipPressed()
{
    done(R_AUTO_SKIP);
}

void RenameDialog::overwritePressed()
{
    if (d->bApplyAll  && d->bApplyAll->isChecked()) {
        done(R_OVERWRITE_ALL);
    } else {
        done(R_OVERWRITE);
    }
}

void RenameDialog::overwriteAllPressed()
{
    done(R_OVERWRITE_ALL);
}

void RenameDialog::resumePressed()
{
    if (d->bApplyAll  && d->bApplyAll->isChecked()) {
        done(R_RESUME_ALL);
    } else {
        done(R_RESUME);
    }
}

void RenameDialog::resumeAllPressed()
{
    done(R_RESUME_ALL);
}

void RenameDialog::updateExistingPressed()
{
    done(R_UPDATE_EXISTING);
}

void RenameDialog::applyAllPressed()
{
    if (d->bApplyAll  && d->bApplyAll->isChecked()) {
        if (d->m_pLineEdit) {
            d->m_pLineEdit->setText(KIO::decodeFileName(d->dest.fileName()));
            d->m_pLineEdit->setEnabled(false);
        }

        if (d->bRename) {
            d->bRename->setEnabled(true);
        }

        if (d->bSuggestNewName) {
            d->bSuggestNewName->setEnabled(false);
        }

        if (d->bUpdate) {
            d->bUpdate->setEnabled(true);
        }
    } else {
        if (d->m_pLineEdit) {
            d->m_pLineEdit->setEnabled(true);
        }

        if (d->bRename) {
            d->bRename->setEnabled(false);
        }

        if (d->bSuggestNewName) {
            d->bSuggestNewName->setEnabled(true);
        }

        if (d->bUpdate) {
            d->bUpdate->setEnabled(false);
        }
    }
}

void RenameDialog::showSrcIcon(const KFileItem& fileitem)
{
    // The preview job failed, show a standard file icon.
    d->m_srcPendingPreview = false;
    d->m_srcPreview->setPixmap(fileitem.pixmap(d->m_srcPreview->height()));
}

void RenameDialog::showDestIcon(const KFileItem& fileitem)
{
    // The preview job failed, show a standard file icon.
    d->m_destPendingPreview = false;
    d->m_destPreview->setPixmap(fileitem.pixmap(d->m_srcPreview->height()));
}

void RenameDialog::showSrcPreview(const KFileItem& fileitem, const QPixmap& pixmap)
{
    Q_UNUSED(fileitem);

    if (d->m_srcPendingPreview) {
        d->m_srcPreview->setPixmap(pixmap);
        d->m_srcPendingPreview = false;
    }
}

void RenameDialog::showDestPreview(const KFileItem& fileitem, const QPixmap& pixmap)
{
    Q_UNUSED(fileitem);

    if (d->m_destPendingPreview) {
        d->m_destPreview->setPixmap(pixmap);
        d->m_destPendingPreview = false;
    }
}

void RenameDialog::resizePanels()
{
    // using QDesktopWidget geometry as Kephal isn't accessible here in kdelibs
    QSize screenSize = QApplication::desktop()->availableGeometry(this).size();
    QSize halfSize = d->m_srcArea->widget()->sizeHint().expandedTo(d->m_destArea->widget()->sizeHint());
    QSize currentSize = d->m_srcArea->size().expandedTo(d->m_destArea->size());
    int maxHeightPossible = screenSize.height() - (size().height() - currentSize.height());
    QSize maxHalfSize = QSize(screenSize.width() / qreal(2.1), maxHeightPossible * qreal(0.9));

    if (halfSize.height() > maxHalfSize.height() &&
            halfSize.width() <= maxHalfSize.width() + d->m_srcArea->verticalScrollBar()->width()) {
        halfSize.rwidth() += d->m_srcArea->verticalScrollBar()->width();
        maxHalfSize.rwidth() += d->m_srcArea->verticalScrollBar()->width();
    }

    d->m_srcArea->setMinimumSize(halfSize.boundedTo(maxHalfSize));
    d->m_destArea->setMinimumSize(halfSize.boundedTo(maxHalfSize));

    KIO::PreviewJob* srcJob = KIO::filePreview(KFileItemList() << d->srcItem,
                              QSize(d->m_srcPreview->width() * qreal(0.9), d->m_srcPreview->height()));
    srcJob->setScaleType(KIO::PreviewJob::Unscaled);

    KIO::PreviewJob* destJob = KIO::filePreview(KFileItemList() << d->destItem,
                               QSize(d->m_destPreview->width() * qreal(0.9), d->m_destPreview->height()));
    destJob->setScaleType(KIO::PreviewJob::Unscaled);

    connect(srcJob, SIGNAL(gotPreview(KFileItem,QPixmap)),
            this, SLOT(showSrcPreview(KFileItem,QPixmap)));
    connect(destJob, SIGNAL(gotPreview(KFileItem,QPixmap)),
            this, SLOT(showDestPreview(KFileItem,QPixmap)));
    connect(srcJob, SIGNAL(failed(KFileItem)),
            this, SLOT(showSrcIcon(KFileItem)));
    connect(destJob, SIGNAL(failed(KFileItem)),
            this, SLOT(showDestIcon(KFileItem)));
}

QScrollArea* RenameDialog::createContainerLayout(QWidget* parent, const KFileItem& item, QLabel* preview)
{
    KFileItemList itemList;
    itemList << item;

    // widget
    KFileMetaDataWidget* metaWidget =  new KFileMetaDataWidget(this);

    metaWidget->setReadOnly(true);
    metaWidget->setItems(itemList);
    connect(metaWidget, SIGNAL(metaDataRequestFinished(KFileItemList)), this, SLOT(resizePanels()));

    // Encapsulate the MetaDataWidgets inside a container with stretch at the bottom.
    // This prevents that the meta data widgets get vertically stretched
    // in the case where the height of m_metaDataArea > m_metaDataWidget.

    QWidget* widgetContainer = new QWidget(parent);
    QVBoxLayout* containerLayout = new QVBoxLayout(widgetContainer);

    containerLayout->setContentsMargins(0, 0, 0, 0);
    containerLayout->setSpacing(0);
    containerLayout->addWidget(preview);
    containerLayout->addWidget(metaWidget);
    containerLayout->addStretch(1);

    QScrollArea* metaDataArea = new QScrollArea(parent);

    metaDataArea->setWidget(widgetContainer);
    metaDataArea->setWidgetResizable(true);
    metaDataArea->setFrameShape(QFrame::NoFrame);

    return metaDataArea;
}

QLabel* RenameDialog::createLabel(QWidget* parent, const QString& text, bool containerTitle)
{
    QLabel* label = new QLabel(parent);

    if (containerTitle) {
        QFont font = label->font();
        font.setBold(true);
        label->setFont(font);
    }

    label->setAlignment(Qt::AlignHCenter);
    label->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    label->setText(text);

    return label;
}

KSqueezedTextLabel* RenameDialog::createSqueezedLabel(QWidget* parent, const QString& text)
{
    KSqueezedTextLabel* label = new KSqueezedTextLabel(text, parent);

    label->setAlignment(Qt::AlignHCenter);
    label->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);

    return label;
}
}
#include "renamedialog.moc"

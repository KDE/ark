/*
 * ark: A program for modifying archives via a GUI.
 *
 * Copyright (C) 2004-2008 Henrique Pinto <henrique.pinto@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#include "arkviewer.h"

#include <KLocale>
#include <KMimeTypeTrader>
#include <KMimeType>
#include <KDebug>
#include <KUrl>
#include <KGlobal>
#include <KIconLoader>
#include <KVBox>
#include <KMessageBox>
#include <KProgressDialog>
#include <QPushButton>
#include <KRun>
#include <KIO/NetAccess>
#include <KHtml/khtml_part.h>

#include <QHBoxLayout>
#include <QFile>
#include <QFrame>
#include <QLabel>
#include <QKeyEvent>

ArkViewer::ArkViewer(QWidget * parent, Qt::WFlags flags)
        : KDialog(parent, flags)
{
    setButtons(Close);
    m_widget = new KVBox(this);
    m_widget->layout()->setSpacing(10);

    setMainWidget(m_widget);

    connect(this, &ArkViewer::finished, this, &ArkViewer::dialogClosed);
}

ArkViewer::~ArkViewer()
{
}

void ArkViewer::dialogClosed()
{
    KConfigGroup conf = KSharedConfig::openConfig()->group("Viewer");
    saveDialogSize(conf);

    if (m_part) {
        KProgressDialog progressDialog
            (this, i18n("Closing preview"),
             i18n("Please wait while the preview is being closed..."));

        progressDialog.setMinimumDuration(500);
        progressDialog.setModal(true);
        progressDialog.setAllowCancel(false);
        progressDialog.progressBar()->setRange(0, 0);

        // #261785: this preview dialog is not modal, so we need to delete
        //          the previewed file ourselves when the dialog is closed;
        //          we used to remove it at the end of ArkViewer::view() when
        //          QDialog::exec() was called instead of QDialog::show().
        const QString previewedFilePath(m_part.data()->url().toDisplayString(QUrl::PreferLocalFile));

        m_part.data()->closeUrl();

        if (!previewedFilePath.isEmpty()) {
            QFile::remove(previewedFilePath);
        }
    }
}

void ArkViewer::view(const QString& fileName, QWidget *parent)
{
    KMimeType::Ptr mimeType = KMimeType::findByPath(fileName);
    kDebug() << "MIME type" << mimeType->name();
    KService::Ptr viewer = ArkViewer::getViewer(mimeType);

    const bool needsExternalViewer = (!viewer &&
                                      !viewer->hasServiceType(QLatin1String("KParts/ReadOnlyPart")));
    if (needsExternalViewer) {
        // We have already resolved the MIME type and the service above.
        // So there is no point in using KRun::runUrl() which would need
        // to do the same again.

        const KUrl::List fileUrlList = KUrl(fileName);
        // The last argument (tempFiles) set to true means that the temporary
        // file will be removed when the viewer application exits.
        KRun::run(*viewer, fileUrlList, parent, true);
        return;
    }

    bool viewInInternalViewer = true;
    if (!viewer) {
        // No internal viewer available for the file.  Ask the user if it
        // should be previewed as text/plain.

        int response;
        if (!mimeType->isDefault()) {
            // File has a defined MIME type, and not the default
            // application/octet-stream.  So it could be viewable as
            // plain text, ask the user.
            response = KMessageBox::warningContinueCancel(parent,
                i18n("The internal viewer cannot preview this type of file<nl/>(%1).<nl/><nl/>Do you want to try to view it as plain text?", mimeType->name()),
                i18nc("@title:window", "Cannot Preview File"),
                KGuiItem(i18nc("@action:button", "Preview as Text"), QIcon::fromTheme(QLatin1String("text-plain"))),
                KStandardGuiItem::cancel(),
                QString(QLatin1String("PreviewAsText_%1")).arg(mimeType->name()));
        }
        else {
            // No defined MIME type, or the default application/octet-stream.
            // There is still a possibility that it could be viewable as plain
            // text, so ask the user.  Not the same as the message/question
            // above, because the wording and default are different.
            response = KMessageBox::warningContinueCancel(parent,
                i18n("The internal viewer cannot preview this unknown type of file.<nl/><nl/>Do you want to try to view it as plain text?"),
                i18nc("@title:window", "Cannot Preview File"),
                KGuiItem(i18nc("@action:button", "Preview as Text"), QIcon::fromTheme(QLatin1String("text-plain"))),
                KStandardGuiItem::cancel(),
                QString(),
                KMessageBox::Dangerous);
        }

        if (response == KMessageBox::Cancel) {
            viewInInternalViewer = false;
        }
        else {						// set for viewer later
            mimeType = KMimeType::mimeType(QLatin1String("text/plain"));
        }
    }

    if (viewInInternalViewer) {
        ArkViewer *internalViewer = new ArkViewer(parent, Qt::Window);
        if (internalViewer->viewInInternalViewer(fileName, mimeType)) {
            internalViewer->show();
            // The internal viewer is showing the file, and will
            // remove the temporary file in dialogClosed().  So there
            // is no more to do here.
            return;
        }
        else {
            KMessageBox::sorry(parent, i18n("The internal viewer cannot preview this file."));
            delete internalViewer;
        }
    }

    // Only get here if there is no internal viewer available or could be
    // used for the file, and no external viewer was opened.  Nothing can be
    // done with the temporary file, so remove it now.
    QFile::remove(fileName);
}

void ArkViewer::keyPressEvent(QKeyEvent *event)
{
    QPushButton *defButton = button(defaultButton());

    // Only handle the event the usual way if the default button has focus
    // Otherwise, pressing enter on KatePart still closes the dialog, for example.
    if ((defButton) && (defButton->hasFocus())) {
        KDialog::keyPressEvent(event);
    }

    event->accept();
}

// This sets the default size of the dialog.  It will only take effect in the case
// where there is no saved size in the config file - it sets the default values
// for KDialog::restoreDialogSize().
QSize ArkViewer::sizeHint() const
{
    return QSize(560, 400);
}

bool ArkViewer::viewInInternalViewer(const QString& fileName, const KMimeType::Ptr& mimeType)
{
    const KUrl fileUrl(fileName);

    setCaption(fileUrl.fileName());
    restoreDialogSize(KSharedConfig::openConfig()->group("Viewer"));

    QFrame *header = new QFrame(m_widget);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);

    QLabel *iconLabel = new QLabel(header);
    headerLayout->addWidget(iconLabel);
    iconLabel->setPixmap(KIconLoader::global()->loadMimeTypeIcon(mimeType->iconName(), KIconLoader::Desktop));
    iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

    KVBox *headerRight = new KVBox(header);
    headerLayout->addWidget(headerRight);
    new QLabel(QString(QLatin1String( "<qt><b>%1</b></qt>" ))
               .arg(fileUrl.fileName()), headerRight
              );
    new QLabel(mimeType->comment(), headerRight);

    header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    m_part = KMimeTypeTrader::self()->createPartInstanceFromQuery<KParts::ReadOnlyPart>(mimeType->name(),
             m_widget,
             this);

    if (!m_part.data()) {
        return false;
    }

    if (m_part.data()->browserExtension()) {
        connect(m_part.data()->browserExtension(), SIGNAL(openUrlRequestDelayed(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)),
                SLOT(slotOpenUrlRequestDelayed(KUrl,KParts::OpenUrlArguments,KParts::BrowserArguments)));
    }

    // #235546
    // TODO: the user should be warned in a non-intrusive way that some features are going to be disabled
    //       maybe there should be an option controlling this
    KHTMLPart *khtmlPart = qobject_cast<KHTMLPart*>(m_part.data());
    if (khtmlPart) {
        kDebug() << "Disabling javascripts, plugins, java and external references for KHTMLPart";
        khtmlPart->setJScriptEnabled(false);
        khtmlPart->setJavaEnabled(false);
        khtmlPart->setPluginsEnabled(false);
        khtmlPart->setMetaRefreshEnabled(false);
        khtmlPart->setOnlyLocalReferences(true);
    }

    m_part.data()->openUrl(fileUrl);

    return true;
}

void ArkViewer::slotOpenUrlRequestDelayed(const KUrl& url, const KParts::OpenUrlArguments& arguments, const KParts::BrowserArguments& browserArguments)
{
    kDebug() << "Opening URL: " << url;

    Q_UNUSED(arguments)
    Q_UNUSED(browserArguments)

    KRun *runner = new KRun(url, 0, false);
    runner->setRunExecutables(false);
}

KService::Ptr ArkViewer::getViewer(const KMimeType::Ptr &mimeType)
{
    // No point in even trying to find anything for application/octet-stream
    if (mimeType->isDefault()) {
        return KService::Ptr();
    }

    // Try to get a read-only kpart for the internal viewer
    KService::List offers = KMimeTypeTrader::self()->query(mimeType->name(), QString::fromLatin1("KParts/ReadOnlyPart"));

    // If we can't find a kpart, try to get an external application
    if (offers.size() == 0) {
        offers = KMimeTypeTrader::self()->query(mimeType->name(), QString::fromLatin1("Application"));
    }

    if (offers.size() > 0) {
        return offers.first();
    } else {
        return KService::Ptr();
    }
}




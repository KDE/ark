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
#include <KPushButton>
#include <KRun>
#include <KIO/NetAccess>
#include <khtml_part.h>

#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>

ArkViewer::ArkViewer(QWidget * parent, Qt::WFlags flags)
        : KDialog(parent, flags)
{
    setButtons(Close);
    m_widget = new KVBox(this);
    m_widget->layout()->setSpacing(10);

    setMainWidget(m_widget);

    connect(this, SIGNAL(finished()), SLOT(dialogClosed()));
}

ArkViewer::~ArkViewer()
{
}

void ArkViewer::dialogClosed()
{
    if (m_part) {
        KProgressDialog progressDialog
            (this, i18n("Closing preview"),
             i18n("Please wait while the preview is being closed..."));

        progressDialog.setMinimumDuration(500);
        progressDialog.setModal(true);
        progressDialog.setAllowCancel(false);
        progressDialog.progressBar()->setRange(0, 0);

        m_part.data()->closeUrl();
    }
}

void ArkViewer::view(const QString& filename, QWidget *parent)
{
    KService::Ptr viewer = ArkViewer::getViewer(filename);

    if (viewer.isNull()) {
        KMessageBox::sorry(parent, i18n("The internal viewer cannot preview this file."));
    } else if (viewer->hasServiceType(QLatin1String( "KParts/ReadOnlyPart" ))) {
        ArkViewer *internalViewer = new ArkViewer(parent, Qt::Window);

        internalViewer->hide();

        if (!internalViewer->viewInInternalViewer(filename)) {
            KMessageBox::sorry(parent, i18n("The internal viewer cannot preview this file."));
            delete internalViewer;
            return;
        }

        internalViewer->show();
    } else { // Try to open it in an external application
        KUrl fileUrl(filename);
        KRun::runUrl(fileUrl, KMimeType::findByUrl(fileUrl, 0, true)->name(), parent);
        return;
    }

    // Unlink the temp file (not used by the external viewer since KRun will do that for us at
    // the right moment
    KIO::NetAccess::del(KUrl(filename), parent);
}

void ArkViewer::keyPressEvent(QKeyEvent *event)
{
    KPushButton *defButton = button(defaultButton());

    // Only handle the event the usual way if the default button has focus
    // Otherwise, pressing enter on KatePart still closes the dialog, for example.
    if ((defButton) && (defButton->hasFocus()))
        KDialog::keyPressEvent(event);

    event->accept();
}

bool ArkViewer::viewInInternalViewer(const QString& filename)
{
    const KUrl fileUrl(filename);

    KMimeType::Ptr mimetype = KMimeType::findByUrl(fileUrl, 0, true);

    setCaption(fileUrl.fileName());
    // TODO: Load the size from the config file
    QSize size = QSize();
    if (size.width() < 200)
        size = QSize(560, 400);
    setInitialSize(size);

    QFrame *header = new QFrame(m_widget);
    QHBoxLayout *headerLayout = new QHBoxLayout(header);

    QLabel *iconLabel = new QLabel(header);
    headerLayout->addWidget(iconLabel);
    iconLabel->setPixmap(KIconLoader::global()->loadMimeTypeIcon(mimetype->iconName(), KIconLoader::Desktop));
    iconLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Minimum);

    KVBox *headerRight = new KVBox(header);
    headerLayout->addWidget(headerRight);
    new QLabel(QString(QLatin1String( "<qt><b>%1</b></qt>" ))
               .arg(fileUrl.fileName()), headerRight
              );
    new QLabel(mimetype->comment(), headerRight);

    header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Maximum);

    m_part = KMimeTypeTrader::self()->createPartInstanceFromQuery<KParts::ReadOnlyPart>(mimetype->name(),
             m_widget,
             this);

    if (!m_part.data()) {
        return false;
    }

    if (m_part.data()->browserExtension()) {
        connect(m_part.data()->browserExtension(),
                SIGNAL(openUrlRequestDelayed(KUrl, KParts::OpenUrlArguments, KParts::BrowserArguments)),
                SLOT(slotOpenUrlRequestDelayed(KUrl, KParts::OpenUrlArguments, KParts::BrowserArguments)));
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

    KRun *runner = new KRun(url, 0, 0, false);
    runner->setRunExecutables(false);
}

KService::Ptr ArkViewer::getViewer(const QString& filename)
{
    KMimeType::Ptr mimetype = KMimeType::findByUrl(KUrl(filename), 0, true);
    // Try to get a read-only kpart for the internal viewer
    KService::List offers = KMimeTypeTrader::self()->query(mimetype->name(), QString::fromLatin1("KParts/ReadOnlyPart"));

    // If we can't find a kpart, try to get an external application
    if (offers.size() == 0) {
        offers = KMimeTypeTrader::self()->query(mimetype->name(), QString::fromLatin1("Application"));
    }

    if (offers.size() > 0) {
        return offers.first();
    } else {
        return KService::Ptr();
    }
}


#include "arkviewer.moc"

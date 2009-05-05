#include "extractHereDndPlugin.h"
#include <QMenu>
#include <krun.h>
#include "kerfuffle/archive.h"
#include "kerfuffle/batchextract.h"

#include <KPluginFactory>
#include <KPluginLoader>
#include <KApplication>
#include <KLocale>

using Kerfuffle::BatchExtract;

K_PLUGIN_FACTORY(ExtractHerePluginFactory,
		registerPlugin<ExtractHereDndPlugin>();
		)
K_EXPORT_PLUGIN(ExtractHerePluginFactory("stupidname"))

void ExtractHereDndPlugin::slotTriggered()
{
	kDebug() << "Preparing job";
	BatchExtract *batchJob = new BatchExtract();

	batchJob->setAutoSubfolder(true);
	batchJob->setDestinationFolder(m_dest.path());
	batchJob->setPreservePaths(true);
	foreach(const KUrl& url, m_info.urlList()) {
		batchJob->addInput(url);
	}

	batchJob->start();
	kDebug() << "Started job";

}

	ExtractHereDndPlugin::ExtractHereDndPlugin(QObject* parent, const QVariantList&)
: KonqDndPopupMenuPlugin(parent)
{
}

void ExtractHereDndPlugin::setup(const KFileItemListProperties& popupMenuInfo,
		KUrl destination,
		QList<QAction*>& userActions)
{

	kDebug() << "plugin setup";
	QString extractHereMessage = i18n("Extract here");

#if 0
	if (!Kerfuffle::supportedMimeTypes().contains(popupMenuInfo.mimeType())) {
		kDebug(1601) << "Unsupported file" << popupMenuInfo.mimeType() << Kerfuffle::supportedMimeTypes();
		return;
	}

	//kDebug() << "Plugin executed" 
		//<< popupMenuInfo.mimeGroup()
		//<< popupMenuInfo.mimeType();

	KAction *action = new KAction("&Extract here", menu);
	connect(action, SIGNAL(triggered()),
			this, SLOT(slotTriggered()));

	menu->addAction(action);
	m_dest = destination;
	m_info = popupMenuInfo;
#endif

}

#include "extractHereDndPlugin.moc"

#ifndef _EXTRACTHEREDNDPLUGIN_H_
#define _EXTRACTHEREDNDPLUGIN_H_

#include "konq_dndpopupmenuplugin.h"
#include <konq_popupmenuinformation.h>
#include <kdebug.h>
#include <kurl.h>

#include <KAction>

class ExtractHereDndPlugin : public KonqDndPopupMenuPlugin
{
	Q_OBJECT

	private slots:
		void slotTriggered();

	public:
		ExtractHereDndPlugin(QObject* parent, const QVariantList&);

		virtual void setup(const KFileItemListProperties& popupMenuInfo,
				KUrl destination,
				QList<QAction*>& userActions);
	private:
		KUrl m_dest;
		QList<KUrl> m_urls;
};

#endif /* _EXTRACTHEREDNDPLUGIN_H_ */

#ifndef PART_H
#define PART_H

#include <KParts/Part>

class ArchiveModel;
class QTreeView;
class KAboutData;

class Part: public KParts::ReadWritePart
{
	Q_OBJECT
	public:
		Part( QWidget *parentWidget, QObject *parent, const QStringList & );
		~Part();
		static KAboutData* createAboutData();

		virtual bool openFile();
		virtual bool saveFile();

	private:
		ArchiveModel *m_model;
		QTreeView    *m_view;

};

#endif // PART_H

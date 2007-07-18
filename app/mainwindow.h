#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <KParts/MainWindow>
#include <KParts/ReadWritePart>
#include <KUrl>

class KRecentFilesAction;

class MainWindow: public KParts::MainWindow
{
	Q_OBJECT
	public:
		MainWindow( QWidget *parent = 0 );
		~MainWindow();

	private slots:
		void openArchive();
		void openUrl( const KUrl& url );
		void quit();

	private:
		void setupActions();

		KParts::ReadWritePart *m_part;
		KRecentFilesAction    *m_recentFilesAction;
};

#endif // MAINWINDOW_H

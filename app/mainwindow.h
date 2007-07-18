#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <KParts/MainWindow>
#include <KParts/ReadWritePart>

class MainWindow: public KParts::MainWindow
{
	Q_OBJECT
	public:
		MainWindow( QWidget *parent = 0 );
		~MainWindow();

	private slots:
		void openArchive();
		void quit();

	private:
		void setupActions();

		KParts::ReadWritePart *m_part;
};

#endif // MAINWINDOW_H

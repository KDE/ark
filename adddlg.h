#ifndef ADDDLG_H
#define ADDDLG_H

#include <qdialog.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qcheckbox.h>

class AddOptionsDlg : public QDialog {
	Q_OBJECT
public:
	AddOptionsDlg( QWidget *parent=0, char *name="" );
	bool onlyUpdate();
	bool storeFullPath();
        void resizeEvent(QResizeEvent *e);
private:
	QPushButton *ok;
	QPushButton *cancel;
	QGroupBox *gb;
	QCheckBox *updatecb;
	QCheckBox *fullcb;
};

#endif ADDDLG_H


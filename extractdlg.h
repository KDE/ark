#ifndef EXTRACTDLG_H
#define EXTRACTDLG_H

#include <stdlib.h>
#include <qdialog.h>
#include <qpushbutton.h>
#include <qgroupbox.h>
#include <qcheckbox.h>
#include <qlineedit.h>
#include <qradiobutton.h>
#include <qbuttongroup.h>
#include <klocale.h>
#include <kapp.h>

class ExtractDlg : public QDialog {
	Q_OBJECT
public:
	ExtractDlg( int eo, QWidget *parent=0, char *name=0 );
        QString getDest();
        bool doOverwrite();
	bool doLowerCase();
	bool doPreservePerms();
	void setMask( unsigned char mask );
	int extractOp();
	enum ExtractOp{ All, Selected, Pattern };
	QString getPattern();

private:
	QLineEdit *le, *le2;
	QCheckBox *cb1, *cb2, *cb3;
	QRadioButton *rb1, *rb2, *rb3;

private slots:
	void browse();
};

#endif

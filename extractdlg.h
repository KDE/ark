#ifndef EXTRACTDLG_H
#define EXTRACTDLG_H

#include <stdlib.h>
#include <qdialog.h>
#include <qpushbt.h>
#include <qgrpbox.h>
#include <qchkbox.h>
#include <qlined.h>
#include <qradiobt.h>
#include <qbttngrp.h>
#include <klocale.h>
#include <kapp.h>

class ExtractDlg : public QDialog {
	Q_OBJECT
public:
	ExtractDlg( int eo, QWidget *parent=0, char *name=0 );
	const char *getDest();
	bool doOverwrite();
	bool doLowerCase();
	bool doPreservePerms();
	void setMask( unsigned char mask );
	int extractOp();
	enum ExtractOp{ All, Selected, Pattern };
	const char *getPattern();

private:
	QLineEdit *le, *le2;
	QCheckBox *cb1, *cb2, *cb3;
	QRadioButton *rb1, *rb2, *rb3;

private slots:
	void browse();
};

#endif /* EXTRACTDLG_H */


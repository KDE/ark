//  -*-C++-*-           emacs magic for .h files

// This file contains a class called Viewer, which exists to help
// separate the archive classes in the backend from the GUI front-end.
// It passes on all its messages to the ArkWidget class for this 
// implementation.
//
// If you need to use the classes, just replace in your own viewer.
//
#ifndef __CVIEWER__H__
#define __CVIEWER__H__

#include <qstringlist.h>
#include "arkwidget.h"

class Viewer
{
public:
  Viewer(ArkWidget * _gui)
    : mGui(_gui) {}
  void add(QStringList *_entries) { mGui->listingAdd(_entries); }
  void setHeaders(QStringList *_headers,
		  int * _rightAlignCols, int _numColsToAlignRight)
    { mGui->setHeaders(_headers, _rightAlignCols, _numColsToAlignRight); }
  int getNumFilesInArchive() { return mGui->getNumFilesInArchive(); }
  int getCol(const QString & _columnHeader)
    { return mGui->getCol(_columnHeader); }
  QString getColData(const QString & _filename, int _col)
    { return mGui->getColData(_filename, _col); }
  
private:
  ArkWidget *mGui;
};

#endif //  __CVIEWER__H__

// $Id$

#include "ktablistbox.h"
#include <qfontmet.h>
#include <qpainter.h>
#include <qkeycode.h>
#include <qpixmap.h>
#include <qapp.h>
#include <qdrawutl.h>
#include <kapp.h>

#include <stdarg.h>

#define INIT_MAX_ITEMS 16

#include "ktablistbox.moc"


int KTabListBox::mMouseCol=-1;
int KTabListBox::mMouseColLeft=0;
int KTabListBox::mMouseColWidth=0;
bool KTabListBox::mMouseAction=FALSE;
QPoint KTabListBox::mMouseStart;


//=============================================================================
//
//  C L A S S   KTabListBoxItem
//
//=============================================================================
KTabListBoxItem::KTabListBoxItem(int aColumns)
{
  columns = aColumns;
  txt = new QString[columns];
  mark = -2;
}


//-----------------------------------------------------------------------------
KTabListBoxItem::~KTabListBoxItem()
{
  if (txt) delete[] txt;
  txt = NULL;
}


//-----------------------------------------------------------------------------
void KTabListBoxItem::setForeground(const QColor& fg)
{
  fgColor = fg;
}


//-----------------------------------------------------------------------------
void KTabListBoxItem::setMarked(int m)
{
  mark = m;
}


//-----------------------------------------------------------------------------
KTabListBoxItem& KTabListBoxItem::operator=(const KTabListBoxItem& from)
{
  int i;

  for (i=0; i<columns; i++)
    txt[i] = from.txt[i];

  fgColor = from.fgColor;

  return *this;
}


//=============================================================================
//
//  C L A S S   KTabListBoxColumn
//
//=============================================================================
//-----------------------------------------------------------------------------
KTabListBoxColumn::KTabListBoxColumn(KTabListBox* pa, int w): QObject()
{
  initMetaObject();
  iwidth = w;
  idefwidth = w;
  colType = KTabListBox::TextColumn;
  parent = pa;
}


//-----------------------------------------------------------------------------
KTabListBoxColumn::~KTabListBoxColumn()
{
}


//-----------------------------------------------------------------------------
void KTabListBoxColumn::setWidth(int w)
{
  iwidth = w;
}


//-----------------------------------------------------------------------------
void KTabListBoxColumn::setDefaultWidth(int w)
{
  idefwidth = w;
}


//-----------------------------------------------------------------------------
void KTabListBoxColumn::setType(KTabListBox::ColumnType lbt)
{
  colType = lbt;
}


//-----------------------------------------------------------------------------
void KTabListBoxColumn::paintCell(QPainter* paint, int row, 
				  const QString& string, bool marked)
{
  const QFontMetrics* fm = &paint->fontMetrics();
  QPixmap* pix = NULL;
  int beg, end, x;

  // p->fillRect(0, 0, cellWidth(col), cellHeight(row), bg);
  if (marked)
  {
    paint->fillRect(0, 0, iwidth, parent->cellHeight(row), 
		     parent->highlightColor);
  }

  if (!string.isEmpty()) 
    switch(colType)
  {
  case KTabListBox::PixmapColumn:
    if (string) pix = parent->dict().find(string);
    if (pix && !pix->isNull())
    {
      paint->drawPixmap(0, 0, *pix);
      break;
    }
    /*else output as string*/

  case KTabListBox::TextColumn:
    paint->drawText(1, fm->ascent() +(fm->leading()), 
		    (const char*)string); 
    break;

  case KTabListBox::MixedColumn:
    QString pixName;
    for (x=0, beg=0; string[beg] == '\t'; x+=parent->tabPixels, beg++)
      ;
    end = beg-1;

    while(string[beg] == '{')
    {
      end = string.find('}', beg+1);
      if (end >= 0)
      {
	pixName = string.mid(beg+1, end-beg-1);
	pix = parent->dict().find(pixName);
	if (!pix)
	{
	  warning("KTabListBox "+QString(name())+
		  ":\nno pixmap for\n`"+pixName+"' registered.");
	} 
	if (!pix->isNull()) paint->drawPixmap(x, 0, *pix);
	x += pix->width()+1;
	beg = end;
      }
      else break;
    }

    paint->drawText(x+1, fm->ascent() +(fm->leading()), 
		    (const char*)string.mid(beg+1, string.length()-beg+1)); 
    break;
  }

  if (marked) 
    paint->fillRect(iwidth-6, 0, iwidth, 128, parent->highlightColor);
  else
    paint->eraseRect(iwidth-6, 0, iwidth, 128);
}


//-----------------------------------------------------------------------------
void KTabListBoxColumn::paint(QPainter* paint)
{
  const QFontMetrics* fm = &paint->fontMetrics();
  paint->drawText(3, fm->ascent() +(fm->leading()),(const char*)name());
}




//=============================================================================
//
//   C L A S S   KTabListBox
//
//=============================================================================
KTabListBox::KTabListBox(QWidget *parent, const char *name, int columns,
			    WFlags f): 
  KTabListBoxInherited(parent, name, f), lbox(this)
{
  const QFontMetrics* fm = &fontMetrics();
  QString f;
  QColorGroup g = colorGroup();

  initMetaObject();

  f = kapp->kdedir();
  f.detach();
  f += "/lib/pics/khtmlw_dnd.xpm";
  dndDefaultPixmap.load(f.data());

  tabPixels = 10;
  maxItems  = 0;
  current   = -1;
  colList   = NULL;
  itemList  = NULL;
  sepChar   = '\n';
  labelHeight = fm->height() + 4;
  columnPadding = fm->height() / 2;
  highlightColor = g.mid();
  mResizeCol = FALSE;
  mSortCol   = -1;
  numColumns = columns;

  //setCursor(sizeHorCursor);
  setMouseTracking(TRUE);

  lbox.setGeometry(0, labelHeight, width(), height()-labelHeight);

  if (columns > 0) setNumCols(columns);
}


//-----------------------------------------------------------------------------
KTabListBox::~KTabListBox()
{
  if (colList)  delete[] colList;
  if (itemList) delete[] itemList;
  colList  = NULL;
  itemList = NULL;
}


//-----------------------------------------------------------------------------
void KTabListBox::setNumRows(int aRows)
{
  lbox.setNumRows(aRows);
}


//-----------------------------------------------------------------------------
void KTabListBox::setTabWidth(int aTabWidth)
{
  tabPixels = aTabWidth;
}


//-----------------------------------------------------------------------------
void KTabListBox::setNumCols(int aCols)
{
  maxItems = 0;

  if (colList) delete[] colList;
  if (itemList) delete[] itemList;
  colList  = NULL;
  itemList = NULL;

  if (aCols < 0) aCols = 0;
  lbox.setNumCols(aCols);
  numColumns = aCols;
  if (aCols <= 0) return;

  colList  = new KTabListBoxColumn[aCols](this);
  itemList = new KTabListBoxItem[INIT_MAX_ITEMS](aCols);
  maxItems = INIT_MAX_ITEMS;
}


//-----------------------------------------------------------------------------
void KTabListBox::readConfig(void)
{
  KConfig* conf = KApplication::getKApplication()->getConfig();
  int beg, end, i, w;
  int cols = numColumns;
  QString str, substr;

  conf->setGroup(name());
  str = conf->readEntry("colwidth");

  if (!str.isEmpty())
    for (i=0, beg=0, end=0; i<cols;)
  {
    end = str.find(',', beg);
    if (end < 0) break;
    w = str.mid(beg,end-beg).toInt();
    colList[i++].setWidth(w);
    beg = end+1;
  }
}


//-----------------------------------------------------------------------------
void KTabListBox::setDefaultColumnWidth(int aWidth, ...)
{
  va_list ap;
  int i, cols;

  cols = numColumns;
  va_start(ap, aWidth);
  for (i=0; aWidth && i<cols; i++)
  {
    colList[i].setDefaultWidth(aWidth);
    aWidth = va_arg(ap, int);
  }
  va_end(ap);
}


//-----------------------------------------------------------------------------
void KTabListBox::setColumnWidth(int col, int aWidth)
{
  if (col<0 || col>=numCols()) return;
  colList[col].setWidth(aWidth);
}


//-----------------------------------------------------------------------------
void KTabListBox::setColumn(int col, const char* aName, int aWidth,
			    ColumnType aType)
{
  if (col<0 || col>=numCols()) return;

  setColumnWidth(col,aWidth);
  colList[col].setName(aName);
  colList[col].setType(aType);
  update();
}


//-----------------------------------------------------------------------------
void KTabListBox::setCurrentItem(int idx, int colId)
{
  int i;

  if (idx>=numRows()) return;

  unmarkAll();

  if (idx != current)
  {
    i = current;
    current = idx;

    updateItem(i,FALSE);
  }

  if (current >= 0)
  {
    markItem(idx);
    emit highlighted(current, colId);
  }
}


//-----------------------------------------------------------------------------
bool KTabListBox::isMarked(int idx) const
{
  return (itemList[idx].marked() >= -1);
}


//-----------------------------------------------------------------------------
void KTabListBox::markItem(int idx, int colId)
{
  if (itemList[idx].marked()==colId) return;
  itemList[idx].setMarked(colId);
  updateItem(idx,FALSE);
}


//-----------------------------------------------------------------------------
void KTabListBox::unmarkItem(int idx)
{
  int mark;

  mark = itemList[idx].marked();
  itemList[idx].setMarked(-2);
  if (mark>=-1) updateItem(idx);
}


//-----------------------------------------------------------------------------
void KTabListBox::unmarkAll(void)
{
  int i;

  for (i=numRows()-1; i>=0; i--)
    unmarkItem(i);
}


//-----------------------------------------------------------------------------
const QString& KTabListBox::text(int row, int col) const
{
  const KTabListBoxItem* item = getItem(row);
  static QString str;
  int i, cols;

  if (!item) 
  {
    str = NULL;
    return str;
  }
  if (col >= 0) return item->text(col);

  cols = item->columns - 1;
  for (str="",i=0; i<=cols; i++)
  {
    str += item->text(i);
    if (i<cols) str += sepChar;
  }

  return str;
}


//-----------------------------------------------------------------------------
void KTabListBox::insertItem(const char* aStr, int row)
{
  int i;

  if (row < 0) row = numRows();
  if (row >= maxItems) resizeList();

  for (i=numRows()-1; i>=row; i--)
    itemList[i+1] = itemList[i];

  if (current >= row) current++;

  setNumRows(numRows()+1);
  changeItem(aStr, row);

  if (needsUpdate(row)) lbox.repaint();
}

//-----------------------------------------------------------------------------
void KTabListBox::appendStrList( QStrList const *strLst )
{
  bool update;
  if( strLst == 0 )
    return;
  QStrListIterator it( *strLst );
  update = autoUpdate();
  setAutoUpdate( false );
  for (uint i=0; i<strLst->count();i++)
  {
    insertItem(it.current());
    ++it;
  }
  setAutoUpdate( update );
  lbox.repaint();
}

//-----------------------------------------------------------------------------
void KTabListBox::changeItem(const char* aStr, int row)
{
  char* str;
  char  sepStr[2];
  char* pos;
  int   i;
  KTabListBoxItem* item;

  if (row < 0 || row >= numRows()) return;

  str = new char[strlen(aStr)+2];
  strcpy(str, aStr);

  sepStr[0] = sepChar;
  sepStr[1] = '\0';

  item = &itemList[row];

  pos = strtok(str, sepStr);
  for (i=0; pos && *pos && i<numCols(); i++)
  {
    item->setText(i, pos);
    pos = strtok(NULL, sepStr);
  }
  item->setForeground(black);

  if (needsUpdate(row)) lbox.repaint();

  delete str;
}


//-----------------------------------------------------------------------------
void KTabListBox::changeItemPart(const char* aStr, int row, int col)
{
  if (row < 0 || row >= numRows()) return;
  if (col < 0 || col >= numCols()) return;

  itemList[row].setText(col, aStr);
  if (needsUpdate(row)) lbox.repaint();
}


//-----------------------------------------------------------------------------
void KTabListBox::changeItemColor(const QColor& newColor, int row)
{
  if (row >= numRows()) return;
  if (row < 0) row = numRows()-1;

  itemList[row].setForeground(newColor);
  if (needsUpdate(row)) lbox.repaint();
}


//-----------------------------------------------------------------------------
void KTabListBox::removeItem(int row)
{
  int i, nr;

  if (row < 0 || row >= numRows()) return;
  if (current > row) current--;

  nr = numRows()-1;
  for (i=row; i<nr; i++)
    itemList[i] = itemList[i+1];

  setNumRows(nr);
  if (nr==0) current = -1;

  if (needsUpdate(row)) lbox.repaint();
}


//-----------------------------------------------------------------------------
void KTabListBox::updateItem(int row, bool erase)
{
  int i;

  for (i=numCols()-1; i>=0; i--)
    lbox.updateCell(row, i, erase);
}


//-----------------------------------------------------------------------------
void KTabListBox::clear(void)
{
  int i;

  for (i=numRows()-1; i>=0; i--)
    itemList[i].setMarked(-2);

  setNumRows(0);
  lbox.setTopLeftCell(0,0);
  current = -1;
}


//-----------------------------------------------------------------------------
void KTabListBox::setSeparator(char sep)
{
  sepChar = sep;
}


//-----------------------------------------------------------------------------
void KTabListBox::resizeList(int newNumItems)
{
  KTabListBoxItem* newItemList;
  int i, ih;

  if (newNumItems < 0) newNumItems =(maxItems << 1);
  if (newNumItems < INIT_MAX_ITEMS) newNumItems = INIT_MAX_ITEMS;

  newItemList = new KTabListBoxItem[newNumItems](numCols());

  ih = newNumItems<numRows() ? newNumItems : numRows();
  for (i=ih-1; i>=0; i--)
  {
    newItemList[i] = itemList[i];
  }

  delete[] itemList;
  itemList = newItemList;
  maxItems = newNumItems;

  setNumRows(ih);
}


//-----------------------------------------------------------------------------
void KTabListBox::resizeEvent(QResizeEvent* e)
{
  int i, w;

  KTabListBoxInherited::resizeEvent(e);

  for (i=numCols()-2, w=0; i>=0; i--)
    w += cellWidth(i);

  if (w + cellWidth(numCols()-1) < e->size().width())
  {
    setColumnWidth(numCols()-1, e->size().width() - w);
  }

  lbox.setGeometry(0, labelHeight, e->size().width(), 
		    e->size().height()-labelHeight);
  lbox.reconnectSBSignals();

  repaint();
}


//-----------------------------------------------------------------------------
void KTabListBox::paintEvent(QPaintEvent*)
{
  int i, ih, x, w;
  QPainter paint;
  QWMatrix matrix;
  QRect    clipR;

  ih = numCols();
  x  = -lbox.xOffset();
  matrix.translate(x, 0);

  //if (!isUpdatesEnabled()) return;

  paint.begin(this);
  for (i=0; i<ih; i++)
  {
    w = colList[i].width();

    if (w + x >= 0)
    {
      clipR.setRect(x, 0, w, labelHeight);
      paint.setWorldMatrix(matrix, FALSE);
      paint.setClipRect(clipR);

      colList[i].paint(&paint);
      if (mMouseCol != i)
      {
	qDrawShadePanel(&paint, 0, 0, w, labelHeight, 
			KTabListBoxInherited::colorGroup());
      }
    }
    matrix.translate(w, 0);
    x += w;
  }
  paint.end();  
}


//-----------------------------------------------------------------------------
void KTabListBox::mouseMoveEvent(QMouseEvent* e)
{
  register int i, x, ex;
  bool mayResize = FALSE;

  ex = e->pos().x();

  if ((e->state() & LeftButton))
  {
    if (mResizeCol && abs(mMouseStart.x() - ex) > 4)
      doMouseResizeCol(e);

    else if (!mResizeCol && 
	     (ex < mMouseColLeft || 
	      ex > (mMouseColLeft+mMouseColWidth)))
      doMouseMoveCol(e);

    return;
  }

  if (e->pos().y() <= labelHeight)
  {
    for (i=0, x=0; ; i++)
    {
      if (ex >= x-4 && ex <= x+4)
      {
	mayResize = TRUE;
	break;
      }
      if (i >= numColumns) break;
      x += colList[i].width();
    }
  }

  if (mayResize)
  {
    if (!mResizeCol)
    {
      mResizeCol = TRUE;
      setCursor(sizeHorCursor);
    }
  }
  else
  {
    if (mResizeCol)
    {
      mResizeCol = FALSE;
      setCursor(arrowCursor);
    }
  }
}


//-----------------------------------------------------------------------------
void KTabListBox::mousePressEvent(QMouseEvent* e)
{
  if (e->button() == LeftButton)
  {
    mMouseStart = e->pos();
    mMouseCol = findCol(e->pos().x());
    mMouseColWidth = colList[mMouseCol].width();
    colXPos(mMouseCol, &mMouseColLeft);
    repaint();
  }
}


//-----------------------------------------------------------------------------
void KTabListBox::mouseReleaseEvent(QMouseEvent* e)
{
  if (e->button() == LeftButton)
  {
    if (!mMouseAction)
    {
      // user wants to select the column rather than drag or move it
      if (mMouseCol >= 0) emit headerClicked(mMouseCol);
    }

    mMouseCol = -1;
    mMouseAction = FALSE;
    repaint();
  }
}


//-----------------------------------------------------------------------------
void KTabListBox::doMouseResizeCol(QMouseEvent* )
{
  if (!mMouseAction) mMouseAction = TRUE;
}


//-----------------------------------------------------------------------------
void KTabListBox::doMouseMoveCol(QMouseEvent* )
{
  if (!mMouseAction) mMouseAction = TRUE;
}


//-----------------------------------------------------------------------------
bool KTabListBox::startDrag(int aCol, int aRow, const QPoint& p)
{
  int       dx = -(dndDefaultPixmap.width() >> 1);
  int       dy = -(dndDefaultPixmap.height() >> 1);
  KDNDIcon* icon;
  int       size, type;
  char*	    data;

  if (!prepareForDrag(aCol,aRow, &data, &size, &type)) return FALSE;

  icon = new KDNDIcon(dndDefaultPixmap, p.x()+dx, p.y()+dy);

  KTabListBoxInherited::startDrag(icon, data, size, type, dx, dy);
  return TRUE;
}


//-----------------------------------------------------------------------------
bool KTabListBox::prepareForDrag(int /*aCol*/, int /*aRow*/, 
				 char** /*data*/, int* /*size*/,
				 int* /*type*/)
{
  return FALSE;
}


//-----------------------------------------------------------------------------
void KTabListBox::horSbValue(int /*val*/)
{
  update();
}


//-----------------------------------------------------------------------------
void KTabListBox::horSbSlidingDone()
{
}




//=============================================================================
//
//   C L A S S   KTabListBoxTable
//
//=============================================================================

QPoint KTabListBoxTable::dragStartPos;
int KTabListBoxTable::dragCol = -1;
int KTabListBoxTable::dragRow = -1;
int KTabListBoxTable::selIdx  = -1;


KTabListBoxTable::KTabListBoxTable(KTabListBox *parent):
  KTabListBoxTableInherited(parent)
{
  QFontMetrics fm = fontMetrics();

  initMetaObject();

  dragging = FALSE;

  setTableFlags(Tbl_autoVScrollBar|Tbl_autoHScrollBar|Tbl_smoothVScrolling|
		 Tbl_clipCellPainting);

  switch(style())
  {
  case MotifStyle:
  case WindowsStyle:
    setBackgroundColor(colorGroup().base());
    setFrameStyle( QFrame::WinPanel | QFrame::Sunken );
    break;
  default:
    setLineWidth(1);
    setFrameStyle(QFrame::Panel|QFrame::Plain);
  }

  setCellWidth(0);
  setCellHeight(fm.lineSpacing() + 1);
  setNumRows(0);

  //setCursor(arrowCursor);
  setMouseTracking(FALSE);

  setFocusPolicy(StrongFocus);
}


//-----------------------------------------------------------------------------
KTabListBoxTable::~KTabListBoxTable()
{
}


//-----------------------------------------------------------------------------
void KTabListBoxTable::paintCell(QPainter* p, int row, int col)
{
  KTabListBox*     owner =(KTabListBox*)parentWidget();
  KTabListBoxItem* item  = owner->getItem(row);

  if (!item) return;
  p->setPen(item->foreground());

  owner->colList[col].paintCell(p, row, item->text(col),(item->marked()==-1));
}


//-----------------------------------------------------------------------------
int KTabListBoxTable::cellWidth(int col)
{
  KTabListBox* owner =(KTabListBox*)parentWidget();

  return(owner->colList ? owner->colList[col].width() : 0);
}


//-----------------------------------------------------------------------------
void KTabListBoxTable::mouseDoubleClickEvent(QMouseEvent* e)
{
  KTabListBox* owner =(KTabListBox*)parentWidget();
  int idx, colnr;

  //mouseReleaseEvent(event);
  idx = owner->currentItem();
  colnr = findCol(e->pos().x());
  if (idx >= 0) emit owner->selected(idx,colnr);
}


//-----------------------------------------------------------------------------
void KTabListBoxTable::doItemSelection(QMouseEvent* e, int idx)
{
  KTabListBox* owner =(KTabListBox*)parentWidget();
  int i, di;

  owner->unmarkAll();
  if ((e->state()&ShiftButton)!=0 && owner->currentItem()>=0)
  {
    i  = owner->currentItem();
    di =(i>idx ? -1 : 1);
    while(1)
    {
      owner->markItem(i);
      if (i == idx) break;
      i += di;
    }
  }
  else owner->setCurrentItem(idx);
}


//-----------------------------------------------------------------------------
void KTabListBoxTable::mousePressEvent(QMouseEvent* e)
{
  KTabListBox* owner =(KTabListBox*)parentWidget();
  int row, col;

  row = findRow(e->pos().y());
  col = findCol(e->pos().x());

  if (e->button() == RightButton)
  {
    // handle popup menu
    if (row >= 0 && col >= 0) emit owner->popupMenu(row, col);
    return;
  }
  else if (e->button() == MidButton) return;

  // arm for possible dragging
  dragStartPos = e->pos();
  dragCol = col;
  dragRow = row;

  // handle item highlighting
  if (row >= 0 && owner->getItem(row)->marked() < -1)
  {
    doItemSelection(e, row);
    selIdx = row;
  }
  else selIdx = -1;
}


//-----------------------------------------------------------------------------
void KTabListBoxTable::mouseReleaseEvent(QMouseEvent* e)
{
  KTabListBox* owner =(KTabListBox*)parentWidget();
  int idx;

  if (e->button() != LeftButton) return;

  if (dragging)
  {
    owner->mouseReleaseEvent(e);
    dragRow = dragCol = -1;
    dragging = FALSE;
  }
  else
  {
    idx = findRow(e->pos().y());
    if (idx >= 0 && selIdx < 0)
      doItemSelection(e, idx);
  }
}


//-----------------------------------------------------------------------------
void KTabListBoxTable::mouseMoveEvent(QMouseEvent* e)
{
  KTabListBox* owner =(KTabListBox*)parentWidget();

  if (dragging)
  {
    owner->mouseMoveEvent(e);
    return;
  }

  if ((e->state() &(RightButton|LeftButton|MidButton)) != 0)
  {
    if (dragRow >= 0 && dragCol >= 0 &&
	(abs(e->pos().x()-dragStartPos.x()) >= 5 ||
	 abs(e->pos().y()-dragStartPos.y()) >= 5))
    {
      // we have a liftoff !
      dragging = owner->startDrag(dragCol, dragRow, mapToGlobal(e->pos()));
      return;
    }
  }
}


//-----------------------------------------------------------------------------
void KTabListBoxTable::reconnectSBSignals(void)
{
  QWidget* hsb =(QWidget*)horizontalScrollBar();
  KTabListBox* owner =(KTabListBox*)parentWidget();

  if (!hsb) return;

  connect(hsb, SIGNAL(valueChanged(int)), owner, SLOT(horSbValue(int)));
  connect(hsb, SIGNAL(sliderReleased()), owner, SLOT(horSbSlidingDone()));
}


//-----------------------------------------------------------------------------
void KTabListBoxTable::focusInEvent(QFocusEvent*)
{
  // Just do nothing here to avoid the annoying flicker whick happens due
  // to a redraw() call per default.
}


//-----------------------------------------------------------------------------
void KTabListBoxTable::focusOutEvent(QFocusEvent*)
{
  // Just do nothing here to avoid the annoying flicker whick happens due
  // to a redraw() call per default.
}

/* $Id$
 * A multi column listbox. Requires the Qt widget set.
 */
#ifndef KTabListBox_h
#define KTabListBox_h

#undef del_item
#include <qdict.h>
#include <qtablevw.h>
#include <qcolor.h>
#include <qpixmap.h>
#include <drag.h>
#include <stdlib.h>

#define MAX_SEP_CHARS 16

class KTabListBoxColumn;
class KTabListBoxTable;
class KTabListBoxItem;
class KTabListBox;

typedef QDict<QPixmap> KTabListBoxDict;

//--------------------------------------------------
#define KTabListBoxTableInherited QTableView
class KTabListBoxTable: public QTableView
{
  Q_OBJECT;
  friend KTabListBox;
public:
  KTabListBoxTable(KTabListBox *owner=0);
  virtual ~KTabListBoxTable();

protected:
  virtual void focusInEvent(QFocusEvent*);
  virtual void focusOutEvent(QFocusEvent*);
  virtual void mouseDoubleClickEvent (QMouseEvent*);
  virtual void mousePressEvent (QMouseEvent*);
  virtual void mouseReleaseEvent (QMouseEvent*);
  virtual void mouseMoveEvent (QMouseEvent*);
  virtual void doItemSelection (QMouseEvent*, int idx);

  virtual void paintCell (QPainter*, int row, int col);
  virtual int cellWidth (int col);

  void reconnectSBSignals (void);

  static QPoint dragStartPos;
  static int dragCol, dragRow;
  static int selIdx;
  bool dragging;
};


//--------------------------------------------------
#define KTabListBoxInherited KDNDWidget
class KTabListBox : public KDNDWidget
{
  Q_OBJECT;
  friend KTabListBoxTable;
  friend KTabListBoxColumn;

public:
  enum ColumnType { TextColumn, PixmapColumn, MixedColumn };

  KTabListBox (QWidget *parent=0, const char *name=0, 
	       int columns=1, WFlags f=0);
  virtual ~KTabListBox();

  uint count (void) const { return numRows(); }

  /** Insert a line before given index, using the separator character
    to separate the fields. If no index is given the line is 
    appended at the end. Returns index of inserted item. */
  virtual void insertItem (const char* string, int itemIndex=-1);

  /** Append a QStrList */
  void appendStrList( QStrList const *strLst );

  /** Same as insertItem, but always appends the new item. */
  void appendItem (const char* string) { insertItem(string); }

  /** Change contents of a line using the separator character
    to separate the fields. */
  virtual void changeItem (const char* string, int itemIndex);

  /** Change part of the contents of a line. */
  virtual void changeItemPart (const char* string, int itemIndex, int column);

  /** Change color of line. */
  virtual void changeItemColor (const QColor& color, int itemIndex=-1);

  /** Set/get number of pixels one tab character stands for. Default: 10 */
  int tabWidth(void) const { return tabPixels; }
  virtual void setTabWidth(int);

  /** Returns contents of given row/column. If col is not set the
   contents of the whole row is returned, seperated with the current 
   seperation character. In this case the string returned is a 
   temporary string that will change on the next text() call on any
   KTabListBox object. */
  const QString& text(int idx, int col=-1) const;

  /** Remove one item from the list. */
  virtual void removeItem (int itemIndex);

  /** Remove contents of listbox */
  virtual void clear (void);

  /** Return index of current item */
  int currentItem (void) const { return current; }

  /** Set the current (selected) column. colId is the value that
    is transfered with the selected() signal that is emited. */
  virtual void setCurrentItem (int idx, int colId=-1);

  /** Unmark all items */
  virtual void unmarkAll (void);

  /** Mark/unmark item with index idx. */
  virtual void markItem (int idx, int colId=-1);
  virtual void unmarkItem (int idx);

  /** Returns TRUE if item with given index is marked. */
  virtual bool isMarked (int idx) const;

  /** Find item at given screen y position. */
  int findItem (int yPos) const { return (lbox.findRow(yPos)); }

  /** Returns first item that is currently displayed in the widget. */
  int topItem (void) const { return (lbox.topCell()); }

  /** Change first displayed item by repositioning the visible part
    of the list. */
  void setTopItem (int idx) { lbox.setTopCell(idx); }

  /** Set number of columns. Warning: this *deletes* the contents
    of the listbox. */
  virtual void setNumCols (int);

  /** Set number of rows in the listbox. The contents stays as it is. */
  virtual void setNumRows (int);

  /** See QTableView for a description of the following methods. */
  int numRows (void) const { return lbox.numRows(); }
  int numCols (void) const { return lbox.numCols(); }
  int cellWidth (int col) { return lbox.cellWidth(col); }
  int totalWidth (void) { return lbox.totalWidth(); }
  int cellHeight (int row) { return lbox.cellHeight(row); }
  int totalHeight (void) { return lbox.totalHeight(); }
  int topCell (void) const { return lbox.topCell(); }
  int leftCell (void) const { return lbox.leftCell(); }
  int lastColVisible (void) const { return lbox.lastColVisible(); }
  int lastRowVisible (void) const { return lbox.lastRowVisible(); }
  bool autoUpdate (void) const { return lbox.autoUpdate(); }
  void setAutoUpdate (bool upd) { lbox.setAutoUpdate(upd); }
  void clearTableFlags(uint f=~0) { lbox.clearTableFlags(f); }
  uint tableFlags(void) { return lbox.tableFlags(); }
  bool testTableFlags(uint f) { return lbox.testTableFlags(f); }
  void setTableFlags(uint f) { lbox.setTableFlags(f); }
  int findCol(int x) { return lbox.findCol(x); }
  int findRow(int y) { return lbox.findRow(y); }
  bool colXPos(int col, int* x) { return lbox.colXPos(col,x); }
  bool rowYPos(int row, int* y) { return lbox.rowYPos(row,y); }

  /** Set column caption, width, and type. */
  virtual void setColumn (int col, const char* caption, 
			  int width=0, ColumnType type=TextColumn);

  /** Set/get column width. */
  virtual void setColumnWidth (int col, int width=0);
  int columnWidth (int col) { return lbox.cellWidth(col); }

  /** Set default width of all columns. */
  virtual void setDefaultColumnWidth(int width0, ...);

  /** set separator character, e.g. '\t'. */
  virtual void setSeparator (char sep);

  /** Return separator character. */
  virtual char separator (void) const { return sepChar; }

  /** For convenient access to the dictionary of pictures that this
   listbox understands. */
  KTabListBoxDict& dict (void) { return pixDict; }

  void repaint (void) { QWidget::repaint(); lbox.repaint(); }

  /** Indicates that a drag has started with given item.
    Returns TRUE if we are dragging, FALSE if drag-start failed. */
  bool startDrag(int col, int row, const QPoint& mousePos);

  QPixmap& dndPixmap(void) { return dndDefaultPixmap; }

  /** Read the config file entries in the group with the name of the
    listbox and set the column widths and those. */
  virtual void readConfig(void);

signals:
  /** emited when the current item changes (either via setCurrentItem()
    or via mouse single-click). */
  void highlighted (int Index, int column);

  /** emitted when the user double-clicks into a line. */
  void selected (int Index, int column);

  /** emitted when the user presses the right mouse button over a line. */
  void popupMenu (int Index, int column);

  /** emitted when the user clicks on a column header. */
  void headerClicked (int column);

protected slots:
  void horSbValue(int val);
  void horSbSlidingDone();

protected:
  bool itemVisible (int idx) { return lbox.rowIsVisible(idx); }
  void updateItem (int idx, bool clear = TRUE);
  bool needsUpdate (int id) { return (lbox.autoUpdate() && itemVisible(id)); }

  KTabListBoxItem* getItem (int idx);
  const KTabListBoxItem* getItem (int idx) const;

  virtual void resizeEvent (QResizeEvent*);
  virtual void paintEvent (QPaintEvent*);
  virtual void mouseMoveEvent(QMouseEvent*);
  virtual void mousePressEvent(QMouseEvent*);
  virtual void mouseReleaseEvent(QMouseEvent*);

  /** Resize item array. Per default enlarge it to double size. */
  virtual void resizeList (int newNumItems=-1);

  /** Called to set drag data, size, and type. If this method
    returns FALSE then no drag occurs. */
  virtual bool prepareForDrag (int col, int row, char** data, int* size, 
			       int* type);

  // Internal method that handles resizing of columns with the mouse.
  virtual void doMouseResizeCol(QMouseEvent*);

  // Internal method that handles moving of columns with the mouse.
  virtual void doMouseMoveCol(QMouseEvent*);

  KTabListBoxColumn*	colList;
  KTabListBoxItem*	itemList;
  int			maxItems, numColumns;
  int			current;
  char			sepChar;
  KTabListBoxDict	pixDict;
  KTabListBoxTable	lbox;
  int			labelHeight;
  QPixmap		dndDefaultPixmap;
  int			columnPadding;
  QColor		highlightColor;
  int			tabPixels;
  bool			mResizeCol;
  int			mSortCol;  // selected column for sorting order or -1

  static int		mMouseCol; // column where the mouse action started
				   // (resize, click, reorder)
  static int		mMouseColLeft; // left offset of mouse column
  static int		mMouseColWidth; // width of mouse column
  static QPoint		mMouseStart;
  static bool		mMouseAction;

private:
  // Disabled copy constructor and operator=
  KTabListBox (const KTabListBox &) {}
  KTabListBox& operator= (const KTabListBox&) { return *this; }
};


//--------------------------------------------------
class KTabListBoxItem
{
public:
  KTabListBoxItem(int numColumns=1);
  virtual ~KTabListBoxItem();

  virtual const QString& text(int column) const { return txt[column]; }
  void setText (int column, const char *text) { txt[column] = text; }
  virtual void setForeground (const QColor& color);
  const QColor& foreground (void) { return fgColor; }

  KTabListBoxItem& operator= (const KTabListBoxItem&);

  int marked (void) const { return mark; }
  bool isMarked (void) const { return (mark >= -1); }
  virtual void setMarked (int mark);

private:
  QString* txt;
  int columns;
  QColor fgColor;
  int mark;

  friend class KTabListBox;
};

typedef KTabListBoxItem* KTabListBoxItemPtr;


//--------------------------------------------------
class KTabListBoxColumn: public QObject
{
  Q_OBJECT;

public:
  KTabListBoxColumn (KTabListBox* parent, int w=0);
  virtual ~KTabListBoxColumn();

  int width (void) const { return iwidth; }
  virtual void setWidth (int w);

  int defaultWidth (void) const { return idefwidth; }
  virtual void setDefaultWidth (int w);

  virtual void setType (KTabListBox::ColumnType);
  KTabListBox::ColumnType type (void) const { return colType; }

  virtual void paintCell (QPainter*, int row, const QString& string, 
			  bool marked);
  virtual void paint (QPainter*);

protected:
  int iwidth, idefwidth;
  KTabListBox::ColumnType colType;
  KTabListBox* parent;
};



inline KTabListBoxItem* KTabListBox :: getItem (int idx)
{
  return ((idx>=0 && idx<maxItems) ? &itemList[idx] : (KTabListBoxItem*)NULL);
}

inline const KTabListBoxItem* KTabListBox :: getItem (int idx) const
{
  return ((idx>=0 && idx<maxItems) ? &itemList[idx] : (KTabListBoxItem*)NULL);
}

#endif /*KTabListBox_h*/

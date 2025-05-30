#pragma once

#ifndef TXSHCOLUMN_INCLUDED
#define TXSHCOLUMN_INCLUDED

#include "tcolumnset.h"
#include "tpersist.h"
#include "traster.h"

#include <QPair>
#include <QString>
#include <QMap>
#include <QStack>

#undef DVAPI
#undef DVVAR
#ifdef TOONZLIB_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif

//=============================================================================
// forward declarations
class TXshLevelColumn;
class TXshSoundColumn;
class TXshSoundTextColumn;
class TXshCellColumn;
class TXshPaletteColumn;
class TXshZeraryFxColumn;
class TXshMeshColumn;
class TXshFolderColumn;
class TXsheet;
class TXshCell;
class TFx;

//=============================================================================
//! The TXshColumn class is the base class of column managers in xsheet.
/*!Inherits \b TColumnHeader and \b TPersist.
\n This is an abstract base class inherited by the concrete classes
   \b TXshCellColumn, \b TXshSoundColumn and TXshPaletteColumn.

   The class contains all features about a generic xsheet column and gives
   all methods to access to these information.

   The createEmpty() static method creates an new empty column.

   The getXsheet() function return xsheet to which column belongs, xsheet can be
   changed using the setXsheet().

   The getIcon() function return column icon.

   TXshColumn allows to manage generic column status getStatusWord() and
setStatusWord.
   Particularly it's possible to know if column is a mask isMask(), is lock
isLocked(),
   or if it's view is in modality preview or camera stand, isCamstandVisible()
and
   isPreviewVisible(). It's besides possible ot change this settings using
setMask(),
   lock(), setCamstandVisible() and setPreviewVisible().
*/
//=============================================================================

class DVAPI TXshColumn : public TColumnHeader, public TPersist {
  int m_status;
  TXsheet *m_xsheet;
  int m_colorTag;  // Usato solo in tabkids
  UCHAR m_opacity;

  QStack<int> m_folderId;
  int m_folderSelector;

  QList<std::pair<int, int>> m_loops;

public:
private:
  int m_colorFilterId;

protected:
  enum {
    eCamstandVisible       = 0x1,
    ePreviewVisible        = 0x2,
    eLocked                = 0x8,
    eMasked                = 0x10,
    eCamstandTransparent43 = 0x20,  // obsoleto, solo per retrocompatibilita'
    eInvertedMask          = 0x80,
    eRenderMask            = 0x100, // Obsolete but don't remove or it may impact older scens
    eAlphaLocked           = 0x200
  };

  TRaster32P m_icon;

  /*!
Set \b m_status to \b status.
\sa getStatusWord()
*/
  void setStatusWord(int status);
  /*!
Return \b m_status.
\sa setStatusWord()
*/
  int getStatusWord() const { return m_status; };

public:
  /*!
Constructs a TXshColumn with default value.
*/
  TXshColumn()
      : m_status(0)
      , m_xsheet(0)
      , m_colorTag(0)
      , m_opacity(255)
      , m_colorFilterId(0)  // None
      , m_folderSelector(-1) {
    m_loops.clear();
  }

  enum ColumnType {
    eLevelType = 0,
    eSoundType,
    eSoundTextType,
    eZeraryFxType,
    ePaletteType,
    eMeshType,
    eFolderType
  };

  virtual ColumnType getColumnType() const = 0;

  //! Returns the column type used to store levels of the specified type.
  static ColumnType toColumnType(int levelType);

  //! Returns true if the column can be parent of another.
  bool canBeParent() const;

  //! Creates an empty TXshColumn of the specified column type.
  static TXshColumn *createEmpty(int colType);

  virtual TXshLevelColumn *getLevelColumn() { return 0; }
  virtual TXshSoundColumn *getSoundColumn() { return 0; }
  virtual TXshSoundTextColumn *getSoundTextColumn() { return 0; }
  virtual TXshCellColumn *getCellColumn() { return 0; }
  virtual TXshPaletteColumn *getPaletteColumn() { return 0; }
  virtual TXshZeraryFxColumn *getZeraryFxColumn() { return 0; }
  virtual TXshMeshColumn *getMeshColumn() { return 0; }
  virtual TXshFolderColumn *getFolderColumn() { return 0; }

  virtual int getMaxFrame(bool ignoreLastStop = false) const = 0;

  virtual TXshColumn *clone() const = 0;

  /*!
Return true if camera stand is visible.
\sa setCamstandVisible()
*/
  bool isCamstandVisible() const;
  /*!
Set column status camera stand visibility to \b on.
\sa isCamstandVisible()
*/
  void setCamstandVisible(bool on);

  /*!
Return true if camera stand is transparent.notice: this value is not relevant if
camerastandVisible is off.
\sa setCamstandTransparent()
*/
  UCHAR getOpacity() const;
  /*!
Set column status camera stand transparent to \b on. notice: this value is not
relevant if camerastandVisible is off.
\sa isCamstandTransparent()
*/
  void setOpacity(UCHAR val) { m_opacity = val; }

  // switch to next camera stand state. It's a 3-state-toggle: notvisible ->
  // visible+nottransparent -> visible+transparent
  // void setCamstandNextState();
  /*!
Return true if preview is visible.
\sa setPreviewVisible()
*/
  bool isPreviewVisible() const;
  /*!
Set column status preview to \b on.
\sa isPreviewVisible()
*/
  void setPreviewVisible(bool on);

  /*!
Return true if column is locked.
\sa lock()
*/
  bool isLocked() const;
  /*!
Set column status look to \b on.
\sa isLocked()
*/
  void lock(bool on);

  /*!
Return true if column is a mask.
\sa setMask()
*/
  bool isMask() const;
  bool isInvertedMask() const;

  bool isAlphaLocked() const;
  /*!
Set column status mask to \b on.
\sa isMask()
*/
  void setIsMask(bool on);
  void setInvertedMask(bool on);

  std::vector<TXshColumn *> getColumnMasks(int frame);

  void setAlphaLocked(bool on);

  virtual bool isEmpty() const { return true; }

  /*!
Return \b m_icon.
*/
  virtual TRaster32P getIcon() { return m_icon; }

  virtual bool isCellEmpty(int row) const = 0;

  /*!
If r0=r1=row return false.
*/
  virtual bool getLevelRange(int row, int &r0, int &r1) const = 0;

  virtual int getRange(int &r0, int &r1, bool ignoreLastStop = false) const {
    r0 = 0;
    r1 = -1;
    return 0;
  }

  /*!
Set \b m_xsheet to \b xsheet.
\sa getXsheet()
*/
  virtual void setXsheet(TXsheet *xsheet) { m_xsheet = xsheet; }
  /*!
Return \b m_xsheet.
\sa setXsheet()
*/
  TXsheet *getXsheet() const { return m_xsheet; }

  virtual TFx *getFx() const { return 0; }

  /*!
Return true if column affects the final render
*/
  virtual bool isRendered() const;

  /*!
Return true if column is control.
*/
  virtual bool isControl() const;

  /*!
Return column color tag.
\sa setColorTag()
*/
  int getColorTag() const { return m_colorTag; }  // Usato solo in tabkids
  /*!
Set column color tag to \b colorTag.
\sa getColorTag()
*/
  void setColorTag(int colorTag) {
    m_colorTag = colorTag;
  }  // Usato solo in tabkids

  int getColorFilterId() const;
  void setColorFilterId(int id) { m_colorFilterId = id; }
  void resetColumnProperties();

  // Folder management
  int setFolderId(int value);
  void setFolderId(int value, int position);
  int getFolderId(int position = -1) const;
  QStack<int> getFolderIdStack() const { return m_folderId; }
  void setFolderIdStack(QStack<int> folderIdStack);
  void removeFolderId(int position);
  int removeFolderId();
  bool isInFolder();
  bool isContainedInFolder(int folderId);
  void removeFromAllFolders();
  int folderDepth();

  TXshColumn *getParentFolder(int position = -1) const;
  bool isParentFolderCamstandVisible() const;
  bool isParentFolderPreviewVisible() const;
  bool isParentFolderLocked() const;
  UCHAR getParentFolderOpacity() const;
  int getParentFolderColorFilterId() const;

  bool isColumnVisible();

  bool loadFolderInfo(std::string tagName, TIStream &is);
  void saveFolderInfo(TOStream &os);

  bool hasLoops() const { return !m_loops.isEmpty(); }
  QList<std::pair<int, int>> getLoops() const { return m_loops; }
  void setLoops(QList<std::pair<int, int>> loops) { m_loops = loops; }
  void addLoop(std::pair<int, int> loop) { m_loops.push_back(loop); }
  void addLoop(int start, int end) { addLoop(std::pair<int, int>(start, end)); }
  void removeLoop(std::pair<int, int> loop) {
    m_loops.erase(m_loops.begin() + m_loops.indexOf(loop));
  }
  void removeLoop(int start, int end) {
    removeLoop(std::pair<int, int>(start, end));
  }
  std::pair<int, int> getLoopForRow(int row);
  std::pair<int, int> getLoopWithRow(int row); 
  bool isInLoopRange(int row);

  bool isLoopedFrame(int row);
  int getLoopedFrame(int row, bool forOnionSkin = false);
  TXshCell getLoopedCell(int row, bool forOnionSkin = false,
                         bool implicitLookup = true);
  bool loadLoopInfo(std::string tagName, TIStream &is);
  void saveLoopInfo(TOStream &os);
};

#ifdef _WIN32
template class DV_EXPORT_API TSmartPointerT<TXshColumn>;
#endif
typedef TSmartPointerT<TXshColumn> TXshColumnP;

//=============================================================================
//! The TXshCellColumn class is the base class of column cell managers in
//! xsheet.
/*!Inherits \b TXshColumn.
\n This is an abstract base class inherited by the concrete classes
   \b TXshLevelColumn and \b TXshZeraryFxColumn.

   The class defines column by cells getCellColumn(). TXshCellColumn is an
object
   composed of a \b TXshCell vector and of an integer to memorize first not
empty cell.

   Class allows to manage cells in a column.
   It's possible to know if cell is empty isCellEmpty(), if column is empty
   isEmpty(), to know column range getRange(), row count getRowCount(), last and
first
   not empty frame getMaxFrame(), getFirstRow().
   Specally class allows to change cells, getCells(), setCells(), or
individually cell,
   getCell(), setCell(); allows to insert insertEmptyCells(), remove
removeCells(),
   clear clearCells() and move moveCells() cells in a column of xsheet.
*/
//=============================================================================

class DVAPI TXshCellColumn : public TXshColumn {
protected:
  std::vector<TXshCell> m_cells;
  int m_first;

  // cell marks information key:frame value:id
  QMap<int, int> m_cellMarkIds;

public:
  /*!
Constructs a TXshCellColumn with default value.
*/
  TXshCellColumn();
  /*!
Destroys the TXshCellColumn object.
*/
  ~TXshCellColumn();

  /*!
Return \b TXshCellColumn.
*/
  TXshCellColumn *getCellColumn() override { return this; }

  virtual bool canSetCell(const TXshCell &cell) const = 0;

  /*!
Return not empty cell range. Set \b r0 and \b r1 to first
and last row with not empty cell.
\sa isEmpty() and getRowCount()
*/
  int getRange(int &r0, int &r1, bool ignoreLastStop = false) const override;
  /*!
Return row count.
\sa isEmpty() and getRange()
*/
  virtual int getRowCount() const;
  /*!
Return true if row count is null.
\sa isCellEmpty(), getRowCount(), getRange()
*/
  bool isEmpty() const override { return getRowCount() == 0; }

  /*!
Return true if cell in \b row is empty.
\sa isEmpty()
*/
  bool isCellEmpty(int row) const override;

  bool isCellImplicit(int row) const;

  /*!
Return cell in \b row.
\sa getCells and setCell()
*/
  virtual const TXshCell &getCell(int row, bool implicitLookup = true,
                                  bool loopedLookup = true) const;
  /*!
Set cell in \b row to \b TXshCell \b cell.
\sa setCells() and getCell(); return false if cannot set cells.
*/
  virtual bool setCell(int row, const TXshCell &cell);

  /*!
Set \b cells[] from \b row to \b row + \b rowCount to column cells.
\sa getCell and setCells()
*/
  virtual void getCells(int row, int rowCount, TXshCell cells[],
                        bool implicitLookup = false, bool loopedLookup = false);
  /*!
Set column cells from \b row to \b row + \b rowCount to cells[].
\sa setCell() and getCell(); return false if cannot set cells[].
*/
  virtual bool setCells(int row, int rowCount, const TXshCell cells[]);

  /*!
Insert \b rowCount empty cells from line \b row.
*/
  virtual void insertEmptyCells(int row, int rowCount = 1);
  /*!
Remove \b rowCount cells from line \b row, with shift.
*/
  virtual void removeCells(int row, int rowCount = 1);
  /*!
Clear \b rowCount cells from line \b row, without shift.
*/
  virtual void clearCells(int row, int rowCount = 1);

  /*!
Return last row with not empty cell.
*/
  int getMaxFrame(bool ignoreLastStop = false) const override;

  /*!
Return first not empty row.
*/
  virtual int getFirstRow() const;
  // debug only
  virtual void checkColumn() const;

  /*!
Return true if cell in row is not empty. Set \b r0 and \b r1 to first and
last row with not empty cell of same level.
*/
  bool getLevelRange(int row, int &r0, int &r1) const override;

  // virtual void updateIcon() = 0;

  void saveCellMarks(TOStream &os);
  bool loadCellMarks(std::string tagName, TIStream &is);
  void setCellMark(int frame, int id);
  int getCellMark(int frame) const;
  QMap<int, int> getCellMarks() const;
  void clearCellMarks();
};

#endif



// TnzBase includes
#include "tfx.h"

// TnzLib includes
#include "toonz/txshcolumn.h"
#include "toonz/txshlevelcolumn.h"
#include "toonz/txshsoundcolumn.h"
#include "toonz/txshpalettecolumn.h"
#include "toonz/txshzeraryfxcolumn.h"
#include "toonz/txshsoundtextcolumn.h"
#include "toonz/txshmeshcolumn.h"
#include "toonz/txshfoldercolumn.h"
#include "toonz/txshcell.h"
#include "toonz/txsheet.h"
#include "toonz/fxdag.h"
#include "toonz/tcolumnfxset.h"
#include "toonz/preferences.h"
#include "toonz/txshleveltypes.h"

#include <QMap>
#include "tstream.h"

namespace {
QMap<int, QPair<QString, TPixel32>> filterColors;
};

//=============================================================================
// TXshCellColumn

TXshCellColumn::TXshCellColumn() : m_first(0) {}

//-----------------------------------------------------------------------------

TXshCellColumn::~TXshCellColumn() { m_cells.clear(); }

//-----------------------------------------------------------------------------

int TXshCellColumn::getRange(int &r0, int &r1, bool ignoreLastStop) const {
  int cellCount = m_cells.size();
  r0            = m_first;
  r1            = r0 + cellCount - 1;
  int i;
  for (i = 0; i < cellCount && m_cells[i].isEmpty(); i++) {
  }
  if (i >= cellCount) {
    r0 = 0;
    r1 = -1;
    return 0;
  }
  r0 = m_first + i;
  for (i = cellCount - 1; i >= 0 && m_cells[i].isEmpty(); i--) {
  }
  r1 = m_first + i;
  if (r1 > r0 && ignoreLastStop &&
      m_cells[cellCount - 1].getFrameId().isStopFrame())
    r1--;
  return r1 - r0 + 1;
}

//-----------------------------------------------------------------------------

int TXshCellColumn::getRowCount() const {
  int i = m_cells.size();
  for (; i > 0 && m_cells[i - 1].isEmpty(); i--) {
  }
  if (i == 0)
    return 0;
  else
    return m_first + i;
}

//-----------------------------------------------------------------------------

int TXshCellColumn::getMaxFrame(bool ignoreLastStop) const {
  int r0, r1;
  getRange(r0, r1, ignoreLastStop);
  return r1;
}

//-----------------------------------------------------------------------------

int TXshCellColumn::getFirstRow() const { return m_first; }

//-----------------------------------------------------------------------------
const TXshCell &TXshCellColumn::getCell(int row, bool implicitLookup,
                                        bool loopedLookup) const {
  static TXshCell emptyCell;

  if (row < 0 || row < m_first || !m_cells.size()) return emptyCell;

  bool implicitEnabled = Preferences::instance()->isImplicitHoldEnabled() &&
                         implicitLookup &&
                         getColumnType() != ColumnType::eSoundTextType &&
                         getColumnType() != ColumnType::eSoundType;

  bool loopsAvailable = loopedLookup && hasLoops();

  int r        = row - m_first;
  bool pastEnd = r >= m_cells.size();
  if (pastEnd) {
    if (!implicitEnabled && !loopsAvailable) return emptyCell;
    r = m_cells.size() - 1;
    if (implicitEnabled && m_cells[r].getFrameId().isStopFrame())
      return emptyCell;
  }

  if (m_cells[r].isEmpty() || pastEnd) {
    if (loopsAvailable) {
      int loopIndex = -1;
      QList<std::pair<int, int>> loops = getLoops();
      for (int i = 0; i < loops.size(); i++) {
        // Not a loop frame if within a loop range
        if (row >= loops.at(i).first && row <= loops.at(i).second) break;
        // Find the preceeding loop closest to the row
        if (row > loops.at(i).second &&
            (loopIndex < 0 || loops.at(i).second > loops.at(loopIndex).second))
          loopIndex = i;
      }

      if (loopIndex != -1) {
        int r0 = getLoops().at(loopIndex).first;
        int r1 = getLoops().at(loopIndex).second;

        // Check between end of loop range and current row to see if it truely
        // is a looped frame.
        for (int r2 = row - 1; r2 > r1; r2--) {
          int adjR = r2 - m_first;
          if (adjR >= m_cells.size()) continue;
          if (!m_cells[adjR].isEmpty()) r0 = r1 = -1;
        }

        if (r1 >= 0 && row >= r0) {
          int totalFrames = r1 - r0 + 1;
          int adjustedRow = row - r0;
          if (adjustedRow < 0) adjustedRow += totalFrames;
          r = (adjustedRow % totalFrames) + r0 - m_first;
          if (r < 0) return emptyCell;
          if (r >= m_cells.size()) r = m_cells.size() - 1;
          if (!m_cells[r].isEmpty()) return m_cells[r];
        }
      }
    }

    if (implicitEnabled) {
      for (; r >= 0; r--) {
        if (m_cells[r].isEmpty()) continue;
        if (m_cells[r].getFrameId().isStopFrame()) return emptyCell;
        break;
      }
      if (r < 0) return emptyCell;
    } 
  }

  return m_cells[r];
}

//-----------------------------------------------------------------------------

bool TXshCellColumn::isCellEmpty(int row) const {
  return getCell(row).isEmpty();
}

//-----------------------------------------------------------------------------

bool TXshCellColumn::isCellImplicit(int row) const {
  if (!Preferences::instance()->isImplicitHoldEnabled() ||
      getColumnType() == ColumnType::eSoundType ||
      getColumnType() == ColumnType::eSoundTextType || row < 0 ||
      row < m_first || getCell(row).isEmpty() ||
      getCell(row).getFrameId().isStopFrame())
    return false;

  if (row > 0 && hasLoops() &&
      getCell(row - 1, false, true) == getCell(row, false, true))
    return true;

  // If we got here, the cell is technically not empty
  // If it is truely empty in the array, then it is implicit
  int r = row - m_first;
  return r >= m_cells.size() || m_cells[r].isEmpty();
}

//-----------------------------------------------------------------------------

//--- debug only
void TXshCellColumn::checkColumn() const {
  assert(m_first >= 0);
  int r0, r1;
  int range = getRange(r0, r1);
  assert(range >= 0);
  assert(range == (int)m_cells.size());
  assert(getRowCount() >= 0);

  TXshCell firstCell = getCell(m_first);
  TXshCell lastCell  = getCell(r1);
  if (range == 0)
    assert(firstCell.isEmpty() && lastCell.isEmpty());
  else {
    assert(firstCell.isEmpty() == 0);
    assert(lastCell.isEmpty() == 0);

    int maxFrame = getMaxFrame();
    assert(maxFrame == r1);
    assert(getCell(maxFrame).isEmpty() == 0);
  }
}

//-----------------------------------------------------------------------------

void TXshCellColumn::getCells(int row, int rowCount, TXshCell cells[],
                              bool implicitLookup, bool loopedLookup) {
  const TXshCell emptyCell;
  int first = m_first;
  int i;
  int cellCount = m_cells.size();
  if (row < 0 || row + rowCount - 1 < first || row >= first + cellCount) {
    for (i = 0; i < rowCount; i++) cells[i] = emptyCell;
    return;
  }

  bool loopsAvailable = loopedLookup && hasLoops();

  int dst, src, n, delta;
  n     = rowCount;
  delta = m_first - row;
  if (delta < 0) {  // le celle cominciano PRIMA della zona da leggere
    dst = 0;
    src = -delta;
  } else {  // le celle cominciano DOPO della zona da leggere
    dst = delta;
    src = 0;
    n -= delta;
  }
  if (n + src > cellCount) n = cellCount - src;

  TXshCell *dstCell    = cells;
  TXshCell *endDstCell = dstCell + dst;
  while (dstCell < endDstCell) *dstCell++ = emptyCell;
  endDstCell += n;
  TXshCell tmpCell;
  while (dstCell < endDstCell) {
    int i = !loopsAvailable ? src : (getLoopedFrame(src + m_first) - m_first);

    TXshCell cell = m_cells[i];
    if (implicitLookup) {
      if (cell.isEmpty() && i > 0 && !tmpCell.isEmpty()) {
        cell = tmpCell;
      } else if (!cell.getFrameId().isStopFrame())
        tmpCell = cell;
    }
    *dstCell++ = cell;
    src++;
  }
  endDstCell = cells + rowCount;
  while (dstCell < endDstCell) {
    if (loopsAvailable && isLoopedFrame(src + m_first)) {
      int i = getLoopedFrame(src + m_first) - m_first;
      TXshCell cell = i >= m_cells.size() ? emptyCell: m_cells[i];
      *dstCell++    = cell;
      src++;
      continue;
    }

    TXshCell cell = m_cells[cellCount - 1];
    if (implicitLookup && !cell.getFrameId().isStopFrame())
      *dstCell++ = cell;
    else
      *dstCell++ = emptyCell;
    src++;
  }
}

//-----------------------------------------------------------------------------

bool TXshCellColumn::setCell(int row, const TXshCell &cell) {
  if (!canSetCell(cell)) return false;
  assert(row >= 0);
#ifndef NDEBUG
  checkColumn();
#endif
  if (m_cells.empty())  // se la colonna e' vuota
  {
    if (!cell.isEmpty()) {
      m_cells.push_back(cell);
      m_first = row;
      // updateIcon();
    }
    return true;
  }

  int oldCellCount = m_cells.size();
  assert(oldCellCount > 0);
  int lastRow = m_first + oldCellCount - 1;

  if (row < m_first)  // prima
  {
    if (cell.isEmpty()) return false;  // non faccio nulla
    int delta = m_first - row;
    assert(delta > 0);
    m_cells.insert(m_cells.begin(), delta - 1, TXshCell());  // celle vuote
    m_cells.insert(m_cells.begin(),
                   cell);  // devo settare la prima comp. del vettore
    m_first = row;         // row 'e la nuova firstrow
// updateIcon();
#ifndef NDEBUG
    checkColumn();
#endif
    return true;
  } else if (row > lastRow)  // dopo
  {
    if (cell.isEmpty()) return false;  // non faccio nulla
    int count = row - lastRow - 1;
    // se necessario, inserisco celle vuote
    for (int i = 0; i < count; ++i) m_cells.push_back(TXshCell());
    m_cells.push_back(cell);
#ifndef NDEBUG
    checkColumn();
#endif
    return true;
  }
  //"[r0,r1]"
  int index = row - m_first;
  assert(0 <= index && index < (int)m_cells.size());

  m_cells[index] = cell;
  // if(index == 0) updateIcon();
  if (cell.isEmpty()) {
    if (row == lastRow) {
      // verifico la presenza di celle bianche alla fine
      while (!m_cells.empty() && m_cells.back().isEmpty()) m_cells.pop_back();
    } else if (row == m_first) {
      // verifico la presenza di celle bianche all'inizio
      while (!m_cells.empty() && m_cells.front().isEmpty()) {
        m_cells.erase(m_cells.begin());
        m_first++;
      }
    }
    if (m_cells.empty()) m_first = 0;
  }
  checkColumn();
  return true;
}

//-----------------------------------------------------------------------------

bool TXshCellColumn::setCells(int row, int rowCount, const TXshCell cells[]) {
  assert(row >= 0);
  int i;
  for (i = 0; i < rowCount; i++)
    if (!canSetCell(cells[i])) return false;

  int oldCellCount = m_cells.size();

  // le celle da settare sono [ra, rb]
  int ra = row;
  int rb = row + rowCount - 1;
  assert(ra <= rb);
  // le celle non vuote sono [c_ra, c_rb]
  int c_rb = m_first + oldCellCount - 1;

  if (row > c_rb)  // sono oltre l'ultima riga
  {
    if (oldCellCount == 0) m_first = row;  // row 'e la nuova firstrow
    int newCellCount = row - m_first + rowCount;
    m_cells.resize(newCellCount);
  } else if (row < m_first) {
    int delta = m_first - row;
    m_cells.insert(m_cells.begin(), delta, TXshCell());
    m_first = row;  // row e' la nuova firstrow
  }
  if (rb > c_rb) {
    for (int i = 0; i < rb - c_rb; ++i) m_cells.push_back(TXshCell());
  }

  int index = row - m_first;
  assert(0 <= index && index < (int)m_cells.size());
  for (int i = 0; i < rowCount; i++) m_cells[index + i] = cells[i];

  // verifico la presenza di celle bianche alla fine
  while (!m_cells.empty() && m_cells.back().isEmpty()) {
    m_cells.pop_back();
  }

  // verifico la presenza di celle bianche all'inizio
  while (!m_cells.empty() && m_cells.front().isEmpty()) {
    m_cells.erase(m_cells.begin());
    m_first++;
  }
  if (m_cells.empty()) {
    m_first = 0;
  }
  // updateIcon();
  return true;
}

//-----------------------------------------------------------------------------

void TXshCellColumn::insertEmptyCells(int row, int rowCount) {
  if (m_cells.empty())
    return;  // se la colonna e' vuota non devo inserire
             // celle

  if (row >= m_first + (int)m_cells.size()) return;  // dopo:non inserisco nulla
  if (row <= m_first)                                // prima
  {
    m_first += rowCount;
  } else  // in mezzo
  {
    int delta                          = row - m_first;
    std::vector<TXshCell>::iterator it = m_cells.begin();
    std::advance(it, delta);
    m_cells.insert(it, rowCount, TXshCell());
  }
}

//-----------------------------------------------------------------------------
// sbianca le celle [row, row+rowCount-1] (senza shiftare)
void TXshCellColumn::clearCells(int row, int rowCount) {
  if (rowCount <= 0) return;
  if (m_cells.empty()) return;  // se la colonna e' vuota

  // le celle da cancellare sono [ra, rb]
  int ra = row;
  int rb = row + rowCount - 1;
  assert(ra <= rb);

  int cellCount = m_cells.size();

  // le celle non vuote sono [c_ra, c_rb]
  int c_ra = m_first;
  int c_rb = m_first + cellCount - 1;
  assert(c_ra <= c_rb);

  // se e' sotto o sopra le celle occupate non devo far nulla
  if (rb < c_ra || ra > c_rb) return;

  // restringo l'area da cancellare in modo che comprenda solo celle non vuote
  if (ra < c_ra) ra = c_ra;
  if (rb > c_rb) rb = c_rb;

  // devo "sbiancare" n celle
  int n = rb - ra + 1;
  assert(n >= 0);

  if (n == cellCount)  // ho cancellato tutto
  {
    m_cells.clear();
    m_first = 0;
  } else {
    assert(ra - c_ra < (int)m_cells.size());
    assert(ra - c_ra + n <= (int)m_cells.size());
    // std::fill_n(&m_cells[ra-c_ra],n, TXshCell());
    int i;
    for (i = 0; i < n; i++) m_cells[ra - c_ra + i] = TXshCell();

    // verifico la presenza di celle bianche alla fine
    while (!m_cells.empty() && m_cells.back().isEmpty()) {
      m_cells.pop_back();
    }

    if (m_cells.empty()) {
      m_first = 0;
    } else {
      // verifico la presenza di celle bianche all'inizio
      while (!m_cells.empty() && m_cells.front().isEmpty()) {
        m_cells.erase(m_cells.begin());
        m_first++;
      }
    }
  }
  // updateIcon();
}

//-----------------------------------------------------------------------------

// rimuove le celle [row, row+rowCount-1] (shiftando)
void TXshCellColumn::removeCells(int row, int rowCount) {
  if (rowCount <= 0) return;
  if (m_cells.empty()) return;  // se la colonna e' vuota

  int cellCount = m_cells.size();

  if (row >= m_first + cellCount) return;  // sono "sotto" l'ultima cella
  if (row < m_first) {
    if (row + rowCount <= m_first)  //"sono sopra la prima cella"
    {                               // aggiorno solo m_first
      m_first -= rowCount;
      return;
    }
    rowCount += row - m_first;
    m_first = row;
  }

  assert(row >= m_first);
  // le celle sotto m_first+cellCount sono gia' vuote
  if (rowCount > m_first + cellCount - row)
    rowCount = m_first + cellCount - row;
  if (rowCount <= 0) return;

  if (row == m_first) {
    // cancello all'inizio
    assert(rowCount <= cellCount);
    std::vector<TXshCell>::iterator it  = m_cells.begin();
    std::vector<TXshCell>::iterator it2 = m_cells.begin();
    std::advance(it2, rowCount);
    m_cells.erase(it, it2);
    // verifico la presenza di celle bianche all'inizio
    while (!m_cells.empty() && m_cells.front().isEmpty()) {
      m_cells.erase(m_cells.begin());
      m_first++;
    }
  } else {
    // cancello dopo l'inizio
    int d                               = row - m_first;
    std::vector<TXshCell>::iterator it  = m_cells.begin();
    std::vector<TXshCell>::iterator it2 = m_cells.begin();
    std::advance(it, d);
    std::advance(it2, d + rowCount);
    m_cells.erase(it, it2);
    if (row + rowCount == m_first + cellCount) {
      // verifico la presenza di celle bianche alla fine
      while (!m_cells.empty() && m_cells.back().isEmpty()) {
        m_cells.pop_back();
      }
    }
  }

  if (m_cells.empty()) {
    m_first = 0;
  }
  // updateIcon();
}

//-----------------------------------------------------------------------------

bool TXshCellColumn::getLevelRange(int row, int &r0, int &r1) const {
  r0 = r1       = row;
  TXshCell cell = getCell(row);
  if (cell.isEmpty()) return false;
  while (r0 > 0 && !isCellImplicit(r0 - 1) &&
         getCell(r0 - 1).m_level.getPointer() == cell.m_level.getPointer())
    r0--;
  while (!isCellImplicit(r1 + 1) &&
         getCell(r1 + 1).m_level.getPointer() == cell.m_level.getPointer())
    r1++;
  return true;
}

//-----------------------------------------------------------------------------

void TXshCellColumn::saveCellMarks(TOStream &os) {
  if (m_cellMarkIds.isEmpty()) return;
  // gather frame numbers with the same id
  QMap<int, QString> idStrMap;
  QMap<int, int>::const_iterator i = m_cellMarkIds.constBegin();
  while (i != m_cellMarkIds.constEnd()) {
    if (!idStrMap.contains(i.value()))
      idStrMap.insert(i.value(), QString::number(i.key()));
    else
      idStrMap[i.value()] += " " + QString::number(i.key());
    ++i;
  }
  os.openChild("cellMarks");
  QMap<int, QString>::const_iterator j = idStrMap.constBegin();
  while (j != idStrMap.constEnd()) {
    std::map<std::string, std::string> attr;
    attr["id"] = std::to_string(j.key());
    os.openChild("cellMark", attr);
    os << j.value();
    os.closeChild();
    ++j;
  }
  os.closeChild();
}

bool TXshCellColumn::loadCellMarks(std::string tagName, TIStream &is) {
  if (tagName != "cellMarks") return false;
  m_cellMarkIds.clear();
  while (is.openChild(tagName)) {
    if (tagName == "cellMark") {
      int id;
      QString frameStr;
      if (is.getTagParam("id", id)) {
        is >> frameStr;
        QStringList frameStrList = frameStr.split(" ", Qt::SkipEmptyParts);
        for (auto fStr : frameStrList) m_cellMarkIds.insert(fStr.toInt(), id);
      }
    }
    is.closeChild();
  }
  return true;
}

void TXshCellColumn::setCellMark(int frame, int id) {
  if (id < 0)
    m_cellMarkIds.remove(frame);
  else
    m_cellMarkIds.insert(frame, id);
}

int TXshCellColumn::getCellMark(int frame) const {
  return m_cellMarkIds.value(frame, -1);
}

QMap<int, int> TXshCellColumn::getCellMarks() const { return m_cellMarkIds; }

void TXshCellColumn::clearCellMarks() { m_cellMarkIds.clear(); }

//=============================================================================
// TXshColumn

void TXshColumn::setStatusWord(int status) { m_status = status; }

//-----------------------------------------------------------------------------

TXshColumn *TXshColumn::createEmpty(int type) {
  switch (type) {
  case eSoundType:
    return new TXshSoundColumn;
  case eZeraryFxType:
    return new TXshZeraryFxColumn(0);
  case ePaletteType:
    return new TXshPaletteColumn;
  case eSoundTextType:
    return new TXshSoundTextColumn;
  case eMeshType:
    return new TXshMeshColumn;
  case eFolderType:
    return new TXshFolderColumn;
  }

  assert(type == eLevelType);
  return new TXshLevelColumn;
}

//-----------------------------------------------------------------------------

TXshColumn::ColumnType TXshColumn::toColumnType(int levelType) {
  ColumnType colType = TXshColumn::eLevelType;

  if (levelType & LEVELCOLUMN_XSHLEVEL)
    colType = TXshColumn::eLevelType;
  else if (levelType == ZERARYFX_XSHLEVEL)
    colType = TXshColumn::eZeraryFxType;
  else if (levelType == PLT_XSHLEVEL)
    colType = TXshColumn::ePaletteType;
  else if (levelType == SND_XSHLEVEL)
    colType = TXshColumn::eSoundType;
  else if (levelType == SND_TXT_XSHLEVEL)
    colType = TXshColumn::eSoundTextType;
  else if (levelType == MESH_XSHLEVEL)
    colType = TXshColumn::eMeshType;
  else if (levelType == FOLDER_XSHLEVEL)
    colType = TXshColumn::eFolderType;
  else
    assert(!"Unknown level type!");

  return colType;
}

//-----------------------------------------------------------------------------

bool TXshColumn::canBeParent() const {
  switch (getColumnType()) {
  case eLevelType:
  case eZeraryFxType:
  case ePaletteType:
  case eMeshType:
    return true;
  case eSoundType:
  case eSoundTextType:
    return false;
  default:
    assert(!"Unknown level type!");
    return false;
  }
}

//-----------------------------------------------------------------------------
bool TXshColumn::isRendered() const {
  //  if (!getXsheet() || !getFx()) return false;
  //  if (!isPreviewVisible()) return false;
  if (!getXsheet() || !isPreviewVisible()) return false;
  if (getColumnType() == eSoundType) return true;
  if (!getFx()) return false;
  return getXsheet()->getFxDag()->isRendered(getFx());
}

/*
bool TXshColumn::isReference() const
{
  if(!getXsheet() || !getFx()) return true;
  if(getXsheet()->getFxDag()->getTerminalFxs()->containsFx(getFx())) return
false;
  return getFx()->getOutputConnectionCount()==0;
}
*/

//-----------------------------------------------------------------------------

bool TXshColumn::isControl() const {
  if (!getXsheet() || !getFx()) return false;
  return getXsheet()->getFxDag()->isControl(getFx());
  /*
TFx *fx = getFx();
if(!fx) return false;
if(!getXsheet()) return false;
if(getXsheet()->getFxDag()->getTerminalFxs()->containsFx(getFx())) return false;
for(int i=0;i<fx->getOutputConnectionCount();i++)
{
TFxPort *port = fx->getOutputConnection(i);
if(port->getOwnerFx()->getInputPort(0) != port)
return true;
}
return false;
*/
}

//-----------------------------------------------------------------------------
#ifdef LEVO
void TXshColumn::setCamstandNextState() {
  setCamstandVisible(!isCamstandVisible());

  if (Preferences::instance()->isCamStandOpacityEnabled()) {
    // camera stand, 3-state-toggle: notvisible -> visible+nottransparent ->
    // visible+transparent
    if (isCamstandVisible()) {
      if (isCamstandTransparent())
        setCamstandVisible(false);
      else
        setCamstandTransparent(true);
    } else {
      setCamstandVisible(true);
      setCamstandTransparent(false);
    }
  } else {
    setCamstandTransparent(false);
    setCamstandVisible(!isCamstandVisible());
  }
}
#endif
//-----------------------------------------------------------------------------

bool TXshColumn::isCamstandVisible() const {
  if (!isParentFolderCamstandVisible()) return false;
  return (m_status & eCamstandVisible) == 0;
}

//-----------------------------------------------------------------------------
/*
bool TXshColumn::isCamstandTransparent() const
{
  return (m_status&eCamstandTransparent)!=0;
}

//-----------------------------------------------------------------------------


void TXshColumn::setCamstandTransparent(bool on)
{
  const int mask = eCamstandTransparent;
  if(!on) m_status&=~mask; else m_status|=mask;
}
*/

void TXshColumn::setCamstandVisible(bool on) {
  const int mask = eCamstandVisible;
  if (on)
    m_status &= ~mask;
  else
    m_status |= mask;
}

//-----------------------------------------------------------------------------

UCHAR TXshColumn::getOpacity() const {
  UCHAR folderOpacity = getParentFolderOpacity();

  return folderOpacity == 255 ? m_opacity : folderOpacity;
}

//-----------------------------------------------------------------------------

int TXshColumn::getColorFilterId() const {
  int folderColorFilterId = getParentFolderColorFilterId();

  return !folderColorFilterId ? m_colorFilterId : folderColorFilterId;
}

//-----------------------------------------------------------------------------

bool TXshColumn::isPreviewVisible() const {
  if (!isParentFolderPreviewVisible()) return false;
  return (m_status & ePreviewVisible) == 0;
}

//-----------------------------------------------------------------------------

void TXshColumn::setPreviewVisible(bool on) {
  const int mask = ePreviewVisible;
  if (on)
    m_status &= ~mask;
  else
    m_status |= mask;
}

//-----------------------------------------------------------------------------

bool TXshColumn::isLocked() const {
  if (isParentFolderLocked()) return true;
  return (m_status & eLocked) != 0;
}

//-----------------------------------------------------------------------------

void TXshColumn::lock(bool on) {
  const int mask = eLocked;
  if (on)
    m_status |= mask;
  else
    m_status &= ~mask;
}

//-----------------------------------------------------------------------------

bool TXshColumn::isMask() const { return (m_status & eMasked) != 0; }

//-----------------------------------------------------------------------------

bool TXshColumn::isInvertedMask() const {
  return (m_status & eInvertedMask) != 0;
}

//-----------------------------------------------------------------------------

bool TXshColumn::isAlphaLocked() const {
  return (m_status & eAlphaLocked) != 0;
}

//-----------------------------------------------------------------------------

void TXshColumn::setIsMask(bool on) {
  const int mask = eMasked;
  if (on)
    m_status |= mask;
  else
    m_status &= ~mask;
}

//-----------------------------------------------------------------------------

void TXshColumn::setInvertedMask(bool on) {
  const int mask = eInvertedMask;
  if (on)
    m_status |= mask;
  else
    m_status &= ~mask;
}

//-----------------------------------------------------------------------------

std::vector<TXshColumn *> TXshColumn::getColumnMasks(int frame) {
  std::vector<TXshColumn *> masks;

  if (m_index < 0 || isEmpty()) return masks;

  TXsheet *xsh = getXsheet();

  // Find the reference column (essentially the mask)
  if (isAlphaLocked()) {
    for (int i = m_index - 1; i >= 0; i--) {
      TXshColumn *rcol = xsh->getColumn(i);

      if (!rcol || rcol->isEmpty()) break;
      // ignore mesh levels, masks and other alpha locked columns
      if (rcol->getColumnType() == TXshColumn::eMeshType ||
          rcol->isAlphaLocked() || rcol->isMask())
        continue;
      if (rcol->getColumnType() == TXshColumn::eLevelType &&
          rcol->isPreviewVisible()) {
        TXshCell cell = xsh->getCell(frame, i);
        if (cell.isEmpty() || cell.getFrameId().isStopFrame()) break;
        masks.push_back(rcol);

        std::vector<TXshColumn *> rcolMasks = rcol->getColumnMasks(frame);
        masks.insert(masks.end(), rcolMasks.begin(), rcolMasks.end());
      }
      break;
    }

    if (!masks.size()) return masks;
  }

  for (int i = m_index + 1; i < xsh->getColumnCount(); i++) {
    TXshColumn *mcol = xsh->getColumn(i);

    if (!mcol || mcol->isEmpty()) break;
    if (mcol->getColumnType() == TXshColumn::eMeshType)
      continue;  // ignore mesh levels
    if (!mcol->isMask() || !mcol->isPreviewVisible()) break;
    TXshCell cell = xsh->getCell(frame, i);
    if (cell.isEmpty() || cell.getFrameId().isStopFrame()) break;
    masks.insert(masks.begin(), mcol);
  }

  return masks;
}

//-----------------------------------------------------------------------------

void TXshColumn::setAlphaLocked(bool on) {
  const int mask = eAlphaLocked;
  if (on)
    m_status |= mask;
  else
    m_status &= ~mask;
}

//-----------------------------------------------------------------------------

void TXshColumn::resetColumnProperties() {
  setStatusWord(0);
  setOpacity(255);
  setColorTag(0);
  setColorFilterId(0);  // None
}


// Folder management

//-----------------------------------------------------------------------------

int TXshColumn::setFolderId(int value) {
  m_folderSelector++;
  m_folderId.insert(m_folderSelector, value);
  return m_folderSelector;
}

//-----------------------------------------------------------------------------

void TXshColumn::setFolderId(int value, int position) {
  assert(position >= 0 && position <= m_folderId.size());
  m_folderId.insert(position, value);
  if (m_folderSelector + 1 >= position) m_folderSelector++;
}

//-----------------------------------------------------------------------------

int TXshColumn::getFolderId(int position) const {
  int pos = position < 0 ? m_folderSelector : position;
  return m_folderId.isEmpty() || pos < 0 || pos >= m_folderId.size()
             ? 0
             : m_folderId[pos];
}

//-----------------------------------------------------------------------------

void TXshColumn::setFolderIdStack(QStack<int> folderIdStack) {
  m_folderId = folderIdStack;
  m_folderSelector = folderIdStack.size() - 1;
}

//-----------------------------------------------------------------------------

void TXshColumn::removeFolderId(int position) {
  if (!isInFolder()) return;
  assert(position >= 0 && position <= m_folderId.size());
  m_folderId.remove(position);
  if (m_folderSelector + 1 >= position && m_folderSelector > -1)
    m_folderSelector--;
}

//-----------------------------------------------------------------------------

int TXshColumn::removeFolderId() {
  m_folderId.remove(m_folderSelector);
  if (m_folderSelector > -1) m_folderSelector--;
  return m_folderSelector + 1;
}

//-----------------------------------------------------------------------------

bool TXshColumn::isInFolder() { return !m_folderId.isEmpty(); }

//-----------------------------------------------------------------------------

bool TXshColumn::isContainedInFolder(int folderId) {
  return m_folderId.contains(folderId);
}

//-----------------------------------------------------------------------------

void TXshColumn::removeFromAllFolders() {
  m_folderId.clear();
  m_folderSelector = -1;
}

//-----------------------------------------------------------------------------

int TXshColumn::folderDepth() { return m_folderId.size(); }

//-----------------------------------------------------------------------------

TXshColumn *TXshColumn::getParentFolder(int position) const {
  TXsheet *xsh = getXsheet();
  if (!xsh) return 0;

  if (m_folderId.isEmpty()) return 0;

  for (int i = getIndex() + 1; i < xsh->getColumnCount(); i++) {
    TXshColumn *folderColumn = xsh->getColumn(i);
    if (folderColumn->getFolderColumn() &&
        folderColumn->getFolderColumn()->getFolderColumnFolderId() ==
            getFolderId(position))
      return folderColumn;
    if (!folderColumn->isInFolder()) break;
  }
  return 0;
}

//-----------------------------------------------------------------------------

bool TXshColumn::isParentFolderCamstandVisible() const {
  if (m_folderId.isEmpty()) return true;
  TXshColumn *column = getParentFolder();

  return column ? column->isCamstandVisible() : true;
}

//-----------------------------------------------------------------------------

bool TXshColumn::isParentFolderPreviewVisible() const {
  if (m_folderId.isEmpty()) return true;
  TXshColumn *column = getParentFolder();

  return column ? column->isPreviewVisible() : true;
}

//-----------------------------------------------------------------------------

bool TXshColumn::isParentFolderLocked() const {
  if (m_folderId.isEmpty()) return false;
  TXshColumn *column = getParentFolder();

  return column ? column->isLocked() : false;
}

//-----------------------------------------------------------------------------

UCHAR TXshColumn::getParentFolderOpacity() const {
  if (m_folderId.isEmpty()) return 255;
  TXshColumn *column = getParentFolder();

  return column ? column->getOpacity() : 255;
}

//-----------------------------------------------------------------------------

int TXshColumn::getParentFolderColorFilterId() const {
  if (m_folderId.isEmpty()) return 0;
  TXshColumn *column = getParentFolder();

  return column ? column->getColorFilterId() : 0;
}

//-----------------------------------------------------------------------------

bool TXshColumn::isColumnVisible() {
  if (m_folderId.isEmpty()) return true;

  for (int i = 0; i < m_folderId.size(); i++) {
    TXshColumn *folder = getParentFolder(i);
    if (folder && !folder->getFolderColumn()->isExpanded()) return false;
  }

  return true;
}

//-----------------------------------------------------------------------------

bool TXshColumn::loadFolderInfo(std::string tagName, TIStream &is) {
  if (tagName != "folderIds") return false;
  m_folderId.clear();
  int folderId;
  while (!is.eos()) {
    is >> folderId;
    m_folderId.push_back(folderId);
    m_folderSelector++;
  }
  return true;
}

//-----------------------------------------------------------------------------

void TXshColumn::saveFolderInfo(TOStream &os) {
  if (m_folderId.isEmpty()) return;

  os.openChild("folderIds");
  for (int i = 0; i < m_folderId.size(); i++) os << m_folderId[i];
  os.closeChild();  // folderIds
}

//-----------------------------------------------------------------------------

std::pair<int, int> TXshColumn::getLoopForRow(int row) {
  if (!hasLoops()) return std::pair<int, int>(-1, -1);

  int loopIndex = -1;
  for (int i = 0; i < m_loops.size(); i++) {
    if (row >= m_loops.at(i).first && row <= m_loops.at(i).second) continue;
    // Find the preceeding loop closest to the row
    if (row > m_loops.at(i).second &&
        (loopIndex < 0 || m_loops.at(i).second > m_loops.at(loopIndex).second))
      loopIndex = i;
  }

  // Not a loop frame if it's before all loops
  if (loopIndex != -1) return m_loops.at(loopIndex);

  return std::pair<int, int>(-1, -1);
}

//-----------------------------------------------------------------------------

std::pair<int, int> TXshColumn::getLoopWithRow(int row) {
  if (!hasLoops()) return std::pair<int, int>(-1, -1);

  for (int i = 0; i < m_loops.size(); i++) {
    if (row >= m_loops.at(i).first && row <= m_loops.at(i).second)
      return m_loops.at(i);
  }

  return std::pair<int, int>(-1, -1);
}

//-----------------------------------------------------------------------------

bool TXshColumn::isInLoopRange(int row) {
  if (!hasLoops()) return false;

  for (int i = 0; i < m_loops.size(); i++) {
    if (row >= m_loops.at(i).first && row <= m_loops.at(i).second) return true;
  }

  return false;
}

//-----------------------------------------------------------------------------

bool TXshColumn::isLoopedFrame(int row) {
  if (!hasLoops()) return false;

  TXshCellColumn *cellColumn = getCellColumn();
  if (cellColumn) {
    // Not a loop frame if it is real
    TXshCell cell = cellColumn->getCell(row, false, false);
    if (!cell.isEmpty()) return false;
  }
  int loopIndex = -1;
  for (int i = 0; i < m_loops.size(); i++) {
    // Not a loop frame if within a loop range
    if (row >= m_loops.at(i).first && row <= m_loops.at(i).second) return false;
    // Find the preceeding loop closest to the row
    if (row > m_loops.at(i).second &&
        (loopIndex < 0 || m_loops.at(i).second > m_loops.at(loopIndex).second))
      loopIndex = i;
  }

  // Not a loop frame if it's before all loops
  if (loopIndex == -1) return false;

  // Check between end of loop range and current row to see if it truely is a
  // looped frame.
  if (cellColumn) {
    for (int r = row - 1; r > m_loops.at(loopIndex).second; r--) {
      TXshCell cell = cellColumn->getCell(r, false, false);
      if (!cell.isEmpty()) return false;
    }
  }

  return true;
}

//-----------------------------------------------------------------------------

int TXshColumn::getLoopedFrame(int row, bool forOnionSkin) {
  if (!hasLoops()) return row;

  TXshCellColumn *cellColumn = getCellColumn();
  if (cellColumn) {
    // Not a loop frame if it is real
    TXshCell cell = cellColumn->getCell(row, false, false);
    if (!cell.isEmpty()) return row;
  }

  int loopIndex = -1;
  for (int i = 0; i < m_loops.size(); i++) {
    // Not a loop frame if withing a loop range
    if (row >= m_loops.at(i).first && row <= m_loops.at(i).second) return row;
    // Find the preceeding loop closest to the row
    if (row > m_loops.at(i).second &&
        (loopIndex < 0 || m_loops.at(i).second > m_loops.at(loopIndex).second))
      loopIndex = i;
  }

  // Not a loop frame if it's before all loops
  if (loopIndex == -1) return row;

  int r0 = m_loops.at(loopIndex).first;
  int r1 = m_loops.at(loopIndex).second;

  if (cellColumn) {
    // Check between end of loop range and current row to see if it truely is a
    // looped frame.  A stop frame between it prevents that from happening
    for (int r = row - 1; r > r1; r--) {
      TXshCell cell = cellColumn->getCell(r, false, false);
      if (!cell.isEmpty()) return row;
    }
  }

  if (r1 < 0 || (!forOnionSkin && row < r0)) return row;

  int totalFrames = r1 - r0 + 1;
  int adjustedRow = row - r0;
  if (adjustedRow < 0) adjustedRow += totalFrames;

  int loopedRow = (adjustedRow % totalFrames) + r0;
  return loopedRow;
}

//-----------------------------------------------------------------------------

TXshCell TXshColumn::getLoopedCell(int row, bool forOnionSkin,
                                   bool implicitLookup) {
  TXshCellColumn *cellColumn = getCellColumn();
  if (!cellColumn) return TXshCell();

  int loopedRow = getLoopedFrame(row, forOnionSkin);

  if (loopedRow < 0) return TXshCell();

  return cellColumn->getCell(loopedRow, implicitLookup);
}

//-----------------------------------------------------------------------------

bool TXshColumn::loadLoopInfo(std::string tagName, TIStream &is) {
  if (tagName != "loops") return false;
  m_loops.clear();
  while (!is.eos()) {
    int startFrame, endFrame;
    is >> startFrame >> endFrame;
    m_loops.push_back(std::pair<int, int>(startFrame, endFrame));
  }
  return true;
}

//-----------------------------------------------------------------------------

void TXshColumn::saveLoopInfo(TOStream &os) {
  if (m_loops.isEmpty()) return;

  os.openChild("loops");
  for (int i = 0; i < m_loops.size(); i++)
    os << m_loops[i].first << m_loops[i].second;
  os.closeChild();  // loops
}

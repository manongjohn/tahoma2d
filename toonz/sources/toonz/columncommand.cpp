

#include "columncommand.h"

// Tnz6 includes
#include "menubarcommandids.h"
#include "columnselection.h"
#include "tapp.h"
#include "expressionreferencemanager.h"

// TnzQt includes
#include "toonzqt/tselectionhandle.h"
#include "toonzqt/stageobjectsdata.h"
#include "toonzqt/menubarcommand.h"
#include "toonzqt/dvdialog.h"
#include "historytypes.h"

// TnzLib includes
#include "toonz/txsheethandle.h"
#include "toonz/tfxhandle.h"
#include "toonz/tobjecthandle.h"
#include "toonz/tscenehandle.h"
#include "toonz/tcolumnhandle.h"
#include "toonz/txsheet.h"
#include "toonz/txshcolumn.h"
#include "toonz/tstageobject.h"
#include "toonz/fxdag.h"
#include "toonz/tcolumnfxset.h"
#include "toonz/tcolumnfx.h"
#include "toonz/tstageobjecttree.h"
#include "toonz/txshzeraryfxcolumn.h"
#include "toonz/txshlevelcolumn.h"
#include "toonz/txshchildlevel.h"
#include "toonz/txshcell.h"
#include "toonz/childstack.h"
#include "toonz/toonzscene.h"
#include "toonz/tcamera.h"
#include "toonz/tstageobjectspline.h"
#include "toonz/fxcommand.h"
#include "toonz/preferences.h"
#include "toonz/tstageobjectid.h"
#include "toonz/columnfan.h"
#include "toonz/txshfoldercolumn.h"

#include "../toonz/xsheetviewer.h"

// TnzBase includes
#include "tfx.h"
#include "tfxattributes.h"
#include "tparamcontainer.h"
#include "tparamset.h"

// TnzCore includes
#include "tstroke.h"
#include "tundo.h"

#include "tools/toolhandle.h"

// Qt includes
#include <QApplication>
#include <QClipboard>
#include <QSet>

#include <memory>

//*************************************************************************
//    Local Namespace  stuff
//*************************************************************************

namespace {

bool canRemoveFx(const std::set<TFx *> &leaves, TFx *fx) {
  bool removeFx = false;
  for (int i = 0; i < fx->getInputPortCount(); i++) {
    TFx *inputFx = fx->getInputPort(i)->getFx();
    if (!inputFx) continue;
    if (leaves.count(inputFx) > 0) {
      removeFx = true;
      continue;
    }
    if (!canRemoveFx(leaves, inputFx)) return false;
    removeFx = true;
  }
  return removeFx;
}

//-----------------------------------------------------------------------------

void getColumnLinkedFxs(TFx *startFx, TFx *newStartFx,
                        QMap<TFx *, TFx *> &fxs) {
  int i, outputNodeCount = 0;
  for (i = 0; i < startFx->getOutputConnectionCount(); i++) {
    TFx *outputFx = startFx->getOutputConnection(i)->getOwnerFx();
    if (fxs.contains(outputFx)) continue;
    if (dynamic_cast<TOutputFx *>(outputFx)) {
      outputNodeCount++;
      continue;
    }
    TFxPort *port = newStartFx->getOutputConnection(i - outputNodeCount);
    if (!port) continue;
    TFx *newOutputFx = port->getOwnerFx();
    fxs[outputFx]    = newOutputFx;
    getColumnLinkedFxs(outputFx, newOutputFx, fxs);
  }
}

//------------------------------------------------------

template <typename ParamCont>
void setGrammerToParams(const ParamCont *cont,
                        const TSyntax::Grammar *grammer) {
  for (int p = 0; p != cont->getParamCount(); ++p) {
    TParam &param = *cont->getParam(p);
    if (TDoubleParam *dp = dynamic_cast<TDoubleParam *>(&param))
      dp->setGrammar(grammer);
    else if (TParamSet *paramSet = dynamic_cast<TParamSet *>(&param))
      setGrammerToParams(paramSet, grammer);
  }
}

//-----------------------------------------------------------------------------

void cloneNotColumnLinkedFxsAndOutputsFx(
    TXsheet *xsh, TXsheet *newXsh, QMap<TFx *, TFx *> *fxTable = nullptr) {
  int columnCount = xsh->getColumnCount();
  assert(newXsh->getColumnCount() == columnCount);

  // Riempio un mapping (fx del vecchio xsheet -> fx del nuovo xsheet)
  QMap<TFx *, TFx *> clonedFxs;
  int i;
  for (i = 0; i < columnCount; i++) {
    TXshColumn *xshColumn = xsh->getColumn(i);

    TFx *columnFx    = xshColumn->getFx();
    TFx *newColumnFx = newXsh->getColumn(i)->getFx();

    if (columnFx && newColumnFx)
      getColumnLinkedFxs(columnFx, newColumnFx, clonedFxs);
  }

  FxDag *fxDag    = xsh->getFxDag();
  FxDag *newFxDag = newXsh->getFxDag();

  // aggiungo nel mapping tutti gli effetti che non sono connessi da un cammino
  // con una colonna
  std::vector<TFx *> fxs, newFxs;
  fxDag->getFxs(fxs);
  newFxDag->getFxs(newFxs);
  QList<TFx *> notColumnLinkedClonedFxs;
  if (fxs.size() > newFxs.size()) {
    for (i = 0; i < fxs.size(); i++) {
      TFx *fx = fxs[i];
      if (clonedFxs.contains(fx)) continue;
      TFx *newFx = fx->clone(false);
      newFxDag->getInternalFxs()->addFx(newFx);
      if (fxDag->getTerminalFxs()->containsFx(fx))
        newFxDag->getTerminalFxs()->addFx(newFx);
      // if the fx has not unique name then let assignUniqueId() set the default
      // name
      if (newFx->getName() == newFx->getFxId()) newFx->setName(L"");
      newFxDag->assignUniqueId(newFx);
      clonedFxs[fx] = newFx;
      notColumnLinkedClonedFxs.append(newFx);
    }
  }

  // Aggiungo tutti gli outputFx mancanti
  for (i = 0; i < fxDag->getOutputFxCount(); i++) {
    if (i >= newFxDag->getOutputFxCount()) newFxDag->addOutputFx();
    newFxDag->getOutputFx(i)->getAttributes()->setDagNodePos(
        fxDag->getOutputFx(i)->getAttributes()->getDagNodePos());
  }

  // connetto tutti i nuovi effetti aggiunti
  for (i = 0; i < notColumnLinkedClonedFxs.size(); i++) {
    TFx *newFx = notColumnLinkedClonedFxs[i];
    TFx *fx    = clonedFxs.key(newFx);
    assert(fx);
    int j;
    for (j = 0; j < fx->getInputPortCount(); j++) {
      TFx *inputFx = fx->getInputPort(j)->getFx();
      if (!inputFx) continue;
      if (dynamic_cast<TXsheetFx *>(inputFx))
        newFx->getInputPort(j)->setFx(newFxDag->getXsheetFx());
      else {
        TFx *newInputFx = clonedFxs[inputFx];
        if (!newInputFx) continue;
        newFx->getInputPort(j)->setFx(newInputFx);
      }
    }
    for (j = 0; j < fx->getOutputConnectionCount(); j++) {
      TFxPort *outputPort = fx->getOutputConnection(j);
      TFx *outputFx       = outputPort->getOwnerFx();
      int k, index = 0;
      if (TOutputFx *outFx = dynamic_cast<TOutputFx *>(outputFx)) continue;

      index = 0;
      for (k = 0; k < outputFx->getInputPortCount(); k++) {
        if (outputFx->getInputPort(k) == outputPort) index = k;
      }
      TFx *newOutputFx = clonedFxs[outputFx];
      newOutputFx->getInputPort(index)->setFx(newFx);
    }
  }
  // Connetto tutti gli output
  for (i = 0; i < fxDag->getOutputFxCount(); i++) {
    TOutputFx *outputFx    = fxDag->getOutputFx(i);
    TOutputFx *newOutputFx = newFxDag->getOutputFx(i);
    TFx *inputFx           = outputFx->getInputPort(0)->getFx();
    if (!inputFx) continue;
    if (dynamic_cast<TXsheetFx *>(inputFx))
      newOutputFx->getInputPort(0)->setFx(newFxDag->getXsheetFx());
    else if (TLevelColumnFx *levelFx =
                 dynamic_cast<TLevelColumnFx *>(inputFx)) {
      int index       = levelFx->getColumnIndex();
      TFx *newInputFx = newXsh->getColumn(index)->getFx();
      newOutputFx->getInputPort(0)->setFx(newInputFx);
    } else if (TZeraryColumnFx *zeraryFx =
                   dynamic_cast<TZeraryColumnFx *>(inputFx)) {
      int index       = zeraryFx->getColumnIndex();
      TFx *newInputFx = newXsh->getColumn(index)->getFx();
      newOutputFx->getInputPort(0)->setFx(newInputFx);
    } else {
      TFx *newInputFx = clonedFxs[inputFx];
      newOutputFx->getInputPort(0)->setFx(newInputFx);
    }
  }

  // reset grammers for all parameters of cloned fxs
  // or they fails to refer to other parameters via expression
  TSyntax::Grammar *grammer = newXsh->getStageObjectTree()->getGrammar();
  QMap<TFx *, TFx *>::const_iterator it;
  for (it = clonedFxs.constBegin(); it != clonedFxs.constEnd(); ++it) {
    setGrammerToParams(it.value()->getParams(), grammer);

    // register to the fx table for expression management
    if (fxTable) fxTable->insert(it.key(), it.value());
  }
}

//-----------------------------------------------------------------------------

void cloneXsheetTStageObjectTree(TXsheet *xsh, TXsheet *newXsh) {
  std::set<TStageObjectId> pegbarIds;
  TStageObjectTree *tree    = xsh->getStageObjectTree();
  TStageObjectTree *newTree = newXsh->getStageObjectTree();
  // Ricostruisco l'intero albero
  int i;
  for (i = 0; i < tree->getStageObjectCount(); i++) {
    TStageObject *stageObject    = tree->getStageObject(i);
    TStageObjectId id            = stageObject->getId();
    TStageObject *newStageObject = newXsh->getStageObject(id);
    if (id.isCamera()) {
      TCamera *camera              = stageObject->getCamera();
      *newStageObject->getCamera() = *camera;
    }
    // Gestisco le spline delle colonne in modo differente perche' sono state
    // gia' settate.
    TStageObjectSpline *spline = newStageObject->getSpline();
    TStageObjectParams *data   = stageObject->getParams();
    newStageObject->assignParams(data);
    delete data;
    newStageObject->setParent(xsh->getStageObjectParent(id));
    if (id.isColumn() && spline) newStageObject->setSpline(spline);

    // Gestisco le spline che non sono di colonna (spline di camera)
    TStageObjectSpline *oldSpline = stageObject->getSpline();
    if (oldSpline && !id.isColumn()) {
      TStageObjectSpline *newSpline = newTree->createSpline();
      newSpline->addRef();
      newSpline->setStroke(new TStroke(*oldSpline->getStroke()));
      newStageObject->setSpline(newSpline);
    }
  }
}

//-----------------------------------------------------------------------------

bool pasteColumnsWithoutUndo(std::set<int> *indices, bool doClone,
                             const StageObjectsData *data) {
  if (!data)
    data = dynamic_cast<const StageObjectsData *>(
        QApplication::clipboard()->mimeData());

  if (!data) return false;

  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();
  // Check Circular References
  if (data->checkCircularReferences(xsh)) {
    DVGui::error(
        QObject::tr("It is not possible to paste the columns: there is a "
                    "circular reference."));
    return false;
  }

  std::list<int> restoredSplineIds;
  data->restoreObjects(*indices, restoredSplineIds, xsh,
                       doClone ? StageObjectsData::eDoClone : 0,
                       TConst::nowhere);
  app->getCurrentXsheet()->notifyXsheetChanged();
  app->getCurrentObject()->notifyObjectIdSwitched();
  return true;
}

//-----------------------------------------------------------------------------

void deleteColumnsWithoutUndo(std::set<int> *indices,
                              bool onlyColumns = false) {
  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  // non posso buttare la colonna di camera
  indices->erase(-1);

  // cerco le fx da eliminare. prima trovo le foglie
  /*-- leavesに消されるカラムのColumnFxを格納していく --*/
  std::set<TFx *> leaves;
  std::set<int>::iterator it2;
  for (it2 = indices->begin(); it2 != indices->end(); ++it2) {
    int index = *it2;
    if (index < 0) continue;
    TXshColumn *column = xsh->getColumn(index);
    if (!column) continue;
    TFx *fx = column->getFx();
    if (fx) leaves.insert(fx);
    if (column->getFolderColumn()) xsh->openCloseFolder(index, true);
  }

  // e poi ...
  /*-- カラムを消した時、一緒に消してもよいFxを格納していく --*/
  std::vector<TFx *> fxsToKill;
  TFxSet *fxSet = xsh->getFxDag()->getInternalFxs();
  int i;
  for (i = 0; i < fxSet->getFxCount(); i++) {
    TFx *fx = fxSet->getFx(i);
    if (canRemoveFx(leaves, fx)) fxsToKill.push_back(fx);
  }

  bool soundColumnRemoved = false;
  int firstIndex          = *indices->begin();
  std::set<int>::reverse_iterator it;
  for (it = indices->rbegin(); it != indices->rend(); ++it) {
    TXshColumn *column = xsh->getColumn(*it);
    if (column && column->getSoundColumn()) soundColumnRemoved = true;
    if (column && column->getFx()) app->getCurrentFx()->setFx(0);
    xsh->removeColumn(*it);
  }

  for (i = 0; i < (int)fxsToKill.size() && !onlyColumns; i++) {
    TFx *fx = fxsToKill[i];

    if (fx == app->getCurrentFx()->getFx()) app->getCurrentFx()->setFx(0);
    if (fx->getLinkedFx() != fx) fx->unlinkParams();

    int j, outputPortCount = fx->getOutputConnectionCount();
    for (j = outputPortCount - 1; j >= 0; j--) {
      TFxPort *port = fx->getOutputConnection(j);

      TFx *outFx = port->getOwnerFx();
      if (TZeraryFx *zeraryFx = dynamic_cast<TZeraryFx *>(outFx))
        outFx = zeraryFx->getColumnFx();

      std::vector<TFx *>::iterator it =
          std::find(fxsToKill.begin(), fxsToKill.end(), outFx);
      std::set<TFx *>::iterator it2 =
          std::find(leaves.begin(), leaves.end(), outFx);

      if (it == fxsToKill.end() && it2 == leaves.end()) port->setFx(0);
    }

    fxSet->removeFx(fx);
    xsh->getFxDag()->getTerminalFxs()->removeFx(fx);
  }

  xsh->updateFrameCount();

  app->getCurrentXsheet()->notifyXsheetChanged();
  app->getCurrentObject()->notifyObjectIdSwitched();
  if (soundColumnRemoved) {
    app->getCurrentScene()->notifyCastChange();
    app->getCurrentXsheet()->notifyXsheetSoundChanged();
  }
  indices->clear();
}

//-----------------------------------------------------------------------------

void resetColumns(
    const QMimeData *mimeData, std::set<int> *indices,
    const QMap<TFxPort *, TFx *> &columnFxLinks,
    const QMap<TStageObjectId, TStageObjectId> &columnObjParents,
    const QMap<TStageObjectId, QList<TStageObjectId>> &columnObjChildren) {
  const StageObjectsData *data =
      dynamic_cast<const StageObjectsData *>(mimeData);
  if (!data) return;
  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();
  std::list<int> restoredSplineIds;
  data->restoreObjects(*indices, restoredSplineIds, xsh, 0);
  QMap<TFxPort *, TFx *>::const_iterator it;
  for (it = columnFxLinks.begin(); it != columnFxLinks.end(); it++)
    it.key()->setFx(it.value());

  // Devo rimettere le stesse connessioni tra gli stage object
  QMap<TStageObjectId, TStageObjectId>::const_iterator it2;
  for (it2 = columnObjParents.begin(); it2 != columnObjParents.end();
       it2++) {  // Parents
    TStageObject *obj = xsh->getStageObject(it2.key());
    if (obj) {
      obj->setParent(it2.value());
    }
  }

  QMap<TStageObjectId, QList<TStageObjectId>>::const_iterator it3;
  for (it3 = columnObjChildren.begin(); it3 != columnObjChildren.end();
       it3++) {  // Children
    QList<TStageObjectId> children = it3.value();
    int i;
    for (i = 0; i < children.size(); i++) {
      TStageObject *child = xsh->getStageObject(children[i]);
      if (child) {
        child->setParent(it3.key());
      }
    }
  }

  app->getCurrentXsheet()->notifyXsheetChanged();
  app->getCurrentObject()->notifyObjectIdSwitched();
}

//-----------------------------------------------------------------------------

// Clones the TXshChildLevel, but NOT any further nested TXshChildLevel inside
// it.
TXshChildLevel *cloneChildLevel(TXshChildLevel *cl) {
  TXshChildLevel *newLevel = new TXshChildLevel(cl->getName());
  newLevel->setScene(cl->getScene());

  TXsheet *childXsh = cl->getXsheet(), *newChildXsh = newLevel->getXsheet();

  std::set<int> indices;
  for (int i = 0; i < childXsh->getColumnCount(); i++) indices.insert(i);

  StageObjectsData *data = new StageObjectsData();
  data->storeColumns(indices, childXsh, 0);
  data->storeColumnFxs(indices, childXsh, 0);
  std::list<int> restoredSplineIds;
  data->restoreObjects(indices, restoredSplineIds, newChildXsh,
                       StageObjectsData::eDoClone);
  delete data;

  cloneNotColumnLinkedFxsAndOutputsFx(childXsh, newChildXsh);
  cloneXsheetTStageObjectTree(childXsh, newChildXsh);

  return newLevel;
}

//-----------------------------------------------------------------------------

void cloneSubXsheets(TXsheet *xsh) {
  std::map<TXsheet *, TXshChildLevel *> visited;
  std::set<TXsheet *> toVisit;

  toVisit.insert(xsh);

  while (!toVisit.empty()) {
    xsh = *toVisit.begin();
    toVisit.erase(xsh);

    for (int i = 0; i < xsh->getColumnCount(); ++i) {
      TXshColumn *column = xsh->getColumn(i);
      if (!column) continue;

      if (TXshCellColumn *cc = column->getCellColumn()) {
        int r0 = 0, r1 = -1;
        cc->getRange(r0, r1);
        if (!cc->isEmpty() && r0 <= r1)
          for (int r = r0; r <= r1; ++r) {
            TXshCell cell = cc->getCell(r);
            if (cell.m_level && cell.m_level->getChildLevel()) {
              TXsheet *subxsh = cell.m_level->getChildLevel()->getXsheet();

              std::map<TXsheet *, TXshChildLevel *>::iterator it =
                  visited.find(subxsh);
              if (it == visited.end()) {
                it = visited
                         .insert(std::make_pair(
                             subxsh,
                             cloneChildLevel(cell.m_level->getChildLevel())))
                         .first;
                toVisit.insert(subxsh);
              }
              assert(it != visited.end());

              cell.m_level = it->second;
              cc->setCell(r, cell);
            }
          }
      }
    }
  }
}

//=============================================================================
//  PasteColumnsUndo
//-----------------------------------------------------------------------------

class PasteColumnsUndo : public TUndo {
  std::set<int> m_indices;
  StageObjectsData *m_data;
  QMap<TFxPort *, TFx *> m_columnLinks;

public:
  PasteColumnsUndo(std::set<int> indices) : m_indices(indices) {
    TApp *app    = TApp::instance();
    m_data       = new StageObjectsData();
    TXsheet *xsh = app->getCurrentXsheet()->getXsheet();
    m_data->storeColumns(indices, xsh, 0);
    m_data->storeColumnFxs(indices, xsh, 0);
    std::set<int>::iterator it;
    for (it = m_indices.begin(); it != m_indices.end(); it++) {
      TXshColumn *column = xsh->getColumn(*it);
      if (!column || !column->getFx()) continue;
      TFx *fx = column->getFx();
      int i;
      for (i = 0; i < fx->getOutputConnectionCount(); i++)
        m_columnLinks[fx->getOutputConnection(i)] = fx;
    }
  }

  ~PasteColumnsUndo() { delete m_data; }

  void undo() const override {
    std::set<int> indices;
    std::set<int>::const_iterator indicesIt = m_indices.begin();
    while (indicesIt != m_indices.end()) indices.insert(*indicesIt++);
    deleteColumnsWithoutUndo(&indices);
  }

  void redo() const override {
    std::set<int> indices;
    std::set<int>::const_iterator indicesIt = m_indices.begin();
    while (indicesIt != m_indices.end()) indices.insert(*indicesIt++);
    resetColumns(m_data, &indices, m_columnLinks,
                 QMap<TStageObjectId, TStageObjectId>(),
                 QMap<TStageObjectId, QList<TStageObjectId>>());
  }

  int getSize() const override { return sizeof(*this); }

  QString getHistoryString() override {
    QString str = QObject::tr("Paste Column :  ");

    std::set<int>::iterator it;
    for (it = m_indices.begin(); it != m_indices.end(); it++) {
      if (it != m_indices.begin()) str += QString(", ");
      str += QString("Col%1").arg(QString::number((*it) + 1));
    }
    return str;
  }

  int getHistoryType() override { return HistoryType::Xsheet; }
};

//=============================================================================
//  DeleteColumnsUndo
//-----------------------------------------------------------------------------

class DeleteColumnsUndo final : public TUndo {
  std::set<int> m_indices;

  QMap<TFxPort *, TFx *> m_columnFxLinks;
  QMap<TStageObjectId, QList<TStageObjectId>> m_columnObjChildren;
  QMap<TStageObjectId, TStageObjectId> m_columnObjParents;

  mutable std::unique_ptr<StageObjectsData> m_data;
  bool m_onlyColumns;

public:
  DeleteColumnsUndo(const std::set<int> &indices, bool onlyColumns)
      : m_indices(indices)
      , m_data(new StageObjectsData)
      , m_onlyColumns(onlyColumns) {
    TApp *app    = TApp::instance();
    TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

    m_data->storeColumns(m_indices, xsh, 0);
    m_data->storeColumnFxs(m_indices, xsh, 0);

    std::set<int>::iterator it;
    for (it = m_indices.begin(); it != m_indices.end(); ++it) {
      TXshColumn *column = xsh->getColumn(*it);
      if (!column) continue;

      // Store output links
      if (TFx *fx = column->getFx()) {
        for (int i = 0; i < fx->getOutputConnectionCount(); ++i) {
          // Only links whose output-connected fx is in the xsheet (ie not
          // temporary)
          // are stored. Temporaries may be erased without an undo
          // notification...

          TFx *outFx = fx->getOutputConnection(i)->getOwnerFx();
          if (xsh->getFxDag()->getInternalFxs()->containsFx(outFx))
            m_columnFxLinks[fx->getOutputConnection(i)] = fx;
        }
      }

      // Store TStageObject children
      int pegbarsCount     = xsh->getStageObjectTree()->getStageObjectCount();
      TStageObjectId id    = TStageObjectId::ColumnId(*it);
      TStageObject *pegbar = xsh->getStageObject(id);
      for (int i = 0; i < pegbarsCount; ++i) {
        TStageObject *other = xsh->getStageObjectTree()->getStageObject(i);
        if (other == pegbar) continue;

        if (other->getParent() == id) {
          // other->setParent(pegbar->getParent());
          m_columnObjChildren[id].append(other->getId());
        }
      }

      // Mi salvo il parent
      m_columnObjParents[id] = pegbar->getParent();
    }
  }

  void redo() const override {
    TXsheet *xsh = TApp::instance()->getCurrentXsheet()->getXsheet();

    assert(!m_data.get());
    m_data.reset(new StageObjectsData);

    m_data->storeColumns(m_indices, xsh, 0);
    m_data->storeColumnFxs(m_indices, xsh, 0);

    std::set<int> indices = m_indices;
    deleteColumnsWithoutUndo(&indices, m_onlyColumns);
  }

  void undo() const override {
    std::set<int> indices = m_indices;
    resetColumns(m_data.get(), &indices, m_columnFxLinks, m_columnObjParents,
                 m_columnObjChildren);

    m_data.reset();
  }

  int getSize() const override { return sizeof(*this); }

  QString getHistoryString() override {
    QString str = QObject::tr("Delete Column :  ");

    std::set<int>::iterator it;
    for (it = m_indices.begin(); it != m_indices.end(); it++) {
      if (it != m_indices.begin()) str += QString(", ");
      str += QString("Col%1").arg(QString::number((*it) + 1));
    }
    return str;
  }

  int getHistoryType() override { return HistoryType::Xsheet; }
};

//---------------------------------------------------------------------------

void doUnifyColumnVisibilityToggles_Recursive(const TXsheet *xsh,
                                              QStringList &modifiedColumnNames,
                                              QList<TXsheet *> doneList,
                                              QStringList &parentXshStack) {
  for (int c = 0; c < xsh->getColumnCount(); c++) {
    TXshColumn *column = xsh->getColumn(c);
    if (!column || column->isEmpty()) continue;
    int colType = column->getColumnType();
    if (colType == TXshColumn::ePaletteType ||
        colType == TXshColumn::eSoundType ||
        colType == TXshColumn::eSoundTextType ||
        colType == TXshColumn::eFolderType)
      continue;
    // visibility check
    if (column->isPreviewVisible() != column->isCamstandVisible()) {
      column->setCamstandVisible(column->isPreviewVisible());
      QString colName = QString::fromStdString(
          xsh->getStageObject(TStageObjectId::ColumnId(c))->getName());
      modifiedColumnNames.append(parentXshStack.join(" > ") + colName);
    }

    // check subxsheet recursively
    TXshCellColumn *cellCol = column->getCellColumn();
    if (!cellCol) continue;
    int r0, r1;
    column->getRange(r0, r1);
    TXshCell cell              = cellCol->getCell(r0);
    TXshChildLevel *childLevel = cell.m_level->getChildLevel();
    if (childLevel) {
      TXsheet *subSheet = childLevel->getXsheet();
      if (!doneList.contains(subSheet)) {
        doneList.append(subSheet);
        parentXshStack.append(QString::fromStdWString(childLevel->getName()));
        doUnifyColumnVisibilityToggles_Recursive(xsh, modifiedColumnNames,
                                                 doneList, parentXshStack);
        parentXshStack.pop_back();
      }
    }
  }
}

}  // namespace

//*************************************************************************
//    Column Command  Undo
//*************************************************************************

class ColumnCommandUndo : public TUndo {
public:
  virtual ~ColumnCommandUndo() {}
  virtual bool isConsistent() const = 0;

protected:
};

//*************************************************************************
//    Insert Empty Columns  Command
//*************************************************************************

class InsertEmptyColumnsUndo final : public ColumnCommandUndo {
  std::vector<std::pair<int, int>> m_columnBlocks;

public:
  InsertEmptyColumnsUndo(const std::vector<int> &indices, bool insertAfter) {
    initialize(indices, insertAfter);
  }

  bool isConsistent() const override { return true; }

  void redo() const override;
  void undo() const override;

  int getSize() const override { return sizeof(*this); }

  QString getHistoryString() override {
    QString str = QObject::tr("Insert Column :  ");

    std::vector<std::pair<int, int>>::iterator it;
    for (it = m_columnBlocks.begin(); it != m_columnBlocks.end(); it++) {
      if (it != m_columnBlocks.begin()) str += QString(", ");
      str += QString("Col%1").arg(QString::number((*it).first + 1));
    }
    return str;
  }

  int getHistoryType() override { return HistoryType::Xsheet; }

private:
  void initialize(const std::vector<int> &indices, bool insertAfter = false);
};

//------------------------------------------------------

void InsertEmptyColumnsUndo::initialize(const std::vector<int> &indices,
                                        bool insertAfter) {
  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  std::vector<int>::const_iterator cb, ce, cEnd = indices.end();

  for (cb = indices.begin(); cb != cEnd; cb = ce)  // As long as block end is ok
  {
    int c = *cb;  // Find a corresponding block start
    for (ce = cb, ++ce, ++c;
         (ce != cEnd) && (*ce == c);)  // by iterating as long as the next
      ++ce, ++c;                       // column index is the previous one + 1

    int insertAt = (insertAfter ? c : *cb);
    m_columnBlocks.push_back(std::make_pair(insertAt, c - *cb));
  }

  assert(!m_columnBlocks.empty());
}

//------------------------------------------------------

void InsertEmptyColumnsUndo::redo() const {
  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  // If this is the very first column, add one now since there is always
  // 1 visible on the screen but its not actually there yet.
  if (!xsh->getColumnCount()) xsh->insertColumn(0);

  std::vector<std::pair<int, int>>::const_reverse_iterator bt,
      bEnd = m_columnBlocks.rend();
  for (bt = m_columnBlocks.rbegin(); bt != bEnd; ++bt)
    for (int n = 0; n != bt->second; ++n) xsh->insertColumn(bt->first);

  app->getCurrentScene()->setDirtyFlag(true);
  app->getCurrentXsheet()->notifyXsheetChanged();
  app->getCurrentObject()->notifyObjectIdSwitched();
}

//------------------------------------------------------

void InsertEmptyColumnsUndo::undo() const {
  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  std::vector<std::pair<int, int>>::const_iterator bt,
      bEnd = m_columnBlocks.end();
  for (bt = m_columnBlocks.begin(); bt != bEnd; ++bt)
    for (int n = 0; n != bt->second; ++n) xsh->removeColumn(bt->first);

  app->getCurrentScene()->setDirtyFlag(true);
  app->getCurrentXsheet()->notifyXsheetChanged();
  app->getCurrentObject()->notifyObjectIdSwitched();
}

//======================================================

void ColumnCmd::insertEmptyColumns(const std::set<int> &indices,
                                   bool insertAfter) {
  if (indices.empty()) return;
  // Filter out all less than 0 indices (in particular, the 'camera' column
  // in the Toonz derivative product "Tab")
  std::vector<int> positiveIndices(indices.begin(), indices.end());
  if (positiveIndices[0] < 0) {
    if (!insertAfter) return;
    // If inserting after on camera column, change it to insert before on column
    // 1
    positiveIndices[0] = 0;
    insertAfter        = false;
  }
  if (positiveIndices.empty()) return;

  std::unique_ptr<ColumnCommandUndo> undo(
      new InsertEmptyColumnsUndo(positiveIndices, insertAfter));
  if (undo->isConsistent()) {
    undo->redo();
    TUndoManager::manager()->add(undo.release());
  }
}

//======================================================

void ColumnCmd::insertEmptyColumn(int index) {
  std::set<int> ii;
  ii.insert(index);
  ColumnCmd::insertEmptyColumns(ii);
}

//*************************************************************************
//    Group Columns  Command
//*************************************************************************

class GroupColumnsUndo final : public ColumnCommandUndo {
  int m_folderId;
  int m_folderCol;
  std::vector<std::pair<int,QStack<int>>> m_oldIndices;

public:
  GroupColumnsUndo(const std::set<int> &indices) { initialize(indices); }

  bool isConsistent() const override { return true; }

  void redo() const override;
  void undo() const override;

  int getSize() const override { return sizeof(*this); }

  QString getHistoryString() override {
    return QObject::tr("Group Columns");
  }

  int getHistoryType() override { return HistoryType::Xsheet; }

private:
  void initialize(const std::set<int> &indices);
};

//------------------------------------------------------

void GroupColumnsUndo::initialize(const std::set<int> &indices) {
  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  m_folderId = xsh->getNewFolderId();
  m_folderCol = *indices.rbegin() + 1;

  std::set<int>::reverse_iterator it;

  for (it = indices.rbegin(); it != indices.rend(); it++) {
    if (*it < 0) continue;
    TXshColumn *column = xsh->getColumn(*it);
    if (!column) continue;

    m_oldIndices.push_back(std::make_pair(*it, column->getFolderIdStack()));
  }
}

//------------------------------------------------------

void GroupColumnsUndo::redo() const {
  if (!m_oldIndices.size()) return;

  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  // Create the new folder column
  xsh->insertColumn(m_folderCol, TXshColumn::eFolderType);

  TStageObject *obj =
      xsh->getStageObject(TStageObjectId::ColumnId(m_folderCol));
  QString str = "Folder" + QString::number(m_folderId);
  obj->setName(str.toStdString());


  TXshFolderColumn *folderColumn =
      xsh->getColumn(m_folderCol)->getFolderColumn();

  folderColumn->setFolderColumnFolderId(m_folderId);

  // Set folder column's folderstack based on 1st (top most) column
  TXshColumn *topColumn   = xsh->getColumn(m_oldIndices[0].first);
  QStack<int> addToFolder = topColumn->getFolderIdStack();

  folderColumn->setFolderIdStack(addToFolder);

  // Build newfolderstack list for each column
  addToFolder.push(m_folderId);

  // Consolidate selection under new column and link them to folder
  int subfolder = -1;
  int col       = m_oldIndices.begin()->first;
  for (int i = 0; i < m_oldIndices.size(); i++) {
    TXshColumn *column = xsh->getColumn(m_oldIndices[i].first);

    if (subfolder >= 0 && !column->isContainedInFolder(subfolder)) {
      addToFolder.pop();
      if (addToFolder.size())
        subfolder = addToFolder.top();
      else
        subfolder = -1;
    }

    column->setFolderId(m_folderId);
    column->setFolderIdStack(addToFolder);
    xsh->moveColumn(m_oldIndices[i].first, col--);

    if (column->getFolderColumn()) {
      subfolder = column->getFolderColumn()->getFolderColumnFolderId();
      addToFolder.push(subfolder);
    }
  }

  app->getCurrentScene()->setDirtyFlag(true);
  app->getCurrentXsheet()->notifyXsheetChanged();
  app->getCurrentObject()->notifyObjectIdSwitched();
}

//------------------------------------------------------

void GroupColumnsUndo::undo() const {
  if (!m_oldIndices.size()) return;

  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  // Move columns back and restore folder links
  int col = m_oldIndices.begin()->first - m_oldIndices.size() + 1;
  for (int i = m_oldIndices.size() - 1; i >= 0; i--) {
    TXshColumn *column = xsh->getColumn(col);

    for (auto o : Orientations::all()) {
      ColumnFan *columnFan = xsh->getColumnFan(o);
      if(!columnFan->isVisible(col)) columnFan->show(col);
    }

    column->setFolderIdStack(m_oldIndices[i].second);

    xsh->moveColumn(col++, m_oldIndices[i].first);
  }

  // Remove the folder
  xsh->removeColumn(m_folderCol);

  app->getCurrentScene()->setDirtyFlag(true);
  app->getCurrentXsheet()->notifyXsheetChanged();
  app->getCurrentObject()->notifyObjectIdSwitched();
}

//======================================================

void ColumnCmd::groupColumns(const std::set<int> &indices) {
  std::set<int> list;

  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  std::set<int>::iterator it;

  for (it = indices.begin(); it != indices.end(); it++) {
    if (*it < 0) continue;
    TXshColumn *column = xsh->getColumn(*it);
    if (!column) continue;

    list.insert(*it);
  }

  if (list.empty()) return;

  std::unique_ptr<ColumnCommandUndo> undo(new GroupColumnsUndo(list));
  if (undo->isConsistent()) {
    undo->redo();
    TUndoManager::manager()->add(undo.release());
  }
}

//*************************************************************************
//    Ungroup Columns  Command
//*************************************************************************

class UngroupColumnsUndo final : public ColumnCommandUndo {
  std::vector<std::pair<int, QStack<int>>> m_oldIndices;
  std::vector<std::pair<int, QStack<int>>> m_newIndices;
  std::vector<int> m_oldFolderIds;

public:
  UngroupColumnsUndo(std::vector<std::pair<int, QStack<int>>> oldIndices,
                     std::vector<std::pair<int, QStack<int>>> newIndices)
      : m_oldIndices(oldIndices), m_newIndices(newIndices) {}

  bool isConsistent() const override { return true; }

  void redo() const override;
  void undo() const override;

  int getSize() const override { return sizeof(*this); }

  QString getHistoryString() override {
    return QObject::tr("Ungroup Columns");
  }

  int getHistoryType() override { return HistoryType::Xsheet; }
};

//------------------------------------------------------

void UngroupColumnsUndo::redo() const {
  if (!m_oldIndices.size()) return;

  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  for (int i = 0; i < m_oldIndices.size(); i++) {
    TXshColumn *column = xsh->getColumn(m_oldIndices[i].first);
    column->setFolderIdStack(m_newIndices[i].second);

    xsh->moveColumn(m_oldIndices[i].first, m_newIndices[i].first);
  }

  app->getCurrentScene()->setDirtyFlag(true);
  app->getCurrentXsheet()->notifyXsheetChanged();
  app->getCurrentObject()->notifyObjectIdSwitched();
}

//------------------------------------------------------

void UngroupColumnsUndo::undo() const {
  if (!m_oldIndices.size()) return;

  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  for (int i = m_oldIndices.size() - 1; i >= 0; i--) {
    xsh->moveColumn(m_newIndices[i].first, m_oldIndices[i].first);

    TXshColumn *column = xsh->getColumn(m_oldIndices[i].first);
    int oldid          = column->getFolderId(); 
    column->setFolderIdStack(m_oldIndices[i].second);
  }

  app->getCurrentScene()->setDirtyFlag(true);
  app->getCurrentXsheet()->notifyXsheetChanged();
  app->getCurrentObject()->notifyObjectIdSwitched();
}

//======================================================

void ColumnCmd::ungroupColumns(const std::set<int> &indices) {
  std::set<int> list;

  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  std::set<int>::iterator it;

  for (it = indices.begin(); it != indices.end(); it++) {
    if (*it < 0) continue;
    TXshColumn *column = xsh->getColumn(*it);
    if (!column || (!column->isInFolder() && !column->getFolderColumn()))
      continue;

    list.insert(*it);
  }

  if (list.empty()) return;

  std::vector<std::pair<int, QStack<int>>> oldIndices;
  std::vector<std::pair<int, QStack<int>>> newIndices;
  std::set<int> removeFolderIds, checkFolders, deleteFolderIds;
  std::set<int> deleteFolders;

  // Ungroup from folder
  std::set<int>::reverse_iterator rit;
  int removeFolderId = 0;
  for (rit = list.rbegin(); rit != list.rend(); rit++) {
    int col            = *rit;
    TXshColumn *column = xsh->getColumn(col);
    bool isFolder      = column->getFolderColumn() ? true : false;

    oldIndices.insert(oldIndices.begin(),
                      std::make_pair(col, column->getFolderIdStack()));

    if (!removeFolderId || !column->isContainedInFolder(removeFolderId))
      removeFolderId =
          isFolder ? column->getFolderColumn()->getFolderColumnFolderId()
                   : column->getFolderId();

    int position = column->getFolderIdStack().indexOf(removeFolderId);
    if (position >= 0)
      column->removeFolderId(position);
    else if (isFolder) {
      column->removeFromAllFolders();

      // Make folder items visible in case it's in a closed folder
      if (!column->getFolderColumn()->isExpanded())
        xsh->openCloseFolder(col, true);
    }
  }

  // Move column out of group to if it no longer belongs to the group
  for (it = list.begin(); it != list.end(); it++) {
    TXshColumn *column = xsh->getColumn(*it);
    int newCol                = *it;
    QStack<int> columnFolders = column->getFolderIdStack();
    bool isFolder             = column->getFolderColumn() ? true : false;

    while (newCol > 0) {
      TXshColumn *prevColumn = xsh->getColumn(newCol - 1);
      int prevFolderId       = prevColumn->getFolderId();
      if (!prevFolderId || column->isContainedInFolder(prevFolderId) ||
          (isFolder &&
           column->getFolderColumn()->getFolderColumnFolderId() ==
               prevFolderId))
        break;
      newCol--;
    }
    newIndices.push_back(std::make_pair(newCol, columnFolders));

    xsh->moveColumn(*it, newCol);

    // if this is a folder, mark to see if we can delete it later
    if (isFolder) {
      bool deleteit = true;
      if (newCol > 0) {
        TXshColumn *priorColumn = xsh->getColumn(newCol - 1);
        if (priorColumn->getFolderId() ==
            column->getFolderColumn()->getFolderColumnFolderId())
          deleteit = false;
      }
      if (deleteit) deleteFolders.insert(newCol);
    }
  }

  if (deleteFolders.size()) TUndoManager::manager()->beginBlock();

  std::unique_ptr<ColumnCommandUndo> undo(new UngroupColumnsUndo(oldIndices, newIndices));
  if (undo->isConsistent())
    TUndoManager::manager()->add(undo.release());

  if (deleteFolders.size()) {
     deleteColumns(deleteFolders, false, false);
    TUndoManager::manager()->endBlock();
  }
}

//*************************************************************************
//    Copy Columns  Command
//*************************************************************************

static void copyColumns_internal(const std::set<int> &indices) {
  assert(!indices.empty());

  StageObjectsData *data = new StageObjectsData;

  TXsheet *xsh = TApp::instance()->getCurrentXsheet()->getXsheet();

  data->storeColumns(
      indices, xsh,
      StageObjectsData::eDoClone | StageObjectsData::eResetFxDagPositions);
  data->storeColumnFxs(
      indices, xsh,
      StageObjectsData::eDoClone | StageObjectsData::eResetFxDagPositions);

  QApplication::clipboard()->setMimeData(data);
}

//------------------------------------------------------

void ColumnCmd::copyColumns(const std::set<int> &indices) {
  if (indices.empty()) return;

  copyColumns_internal(indices);
}

//*************************************************************************
//    Paste Columns  Command
//*************************************************************************

void ColumnCmd::pasteColumns(std::set<int> &indices,
                             const StageObjectsData *data) {
  // indices will be updated here by inserted column ids after pasting
  bool isPaste = pasteColumnsWithoutUndo(&indices, true, data);
  if (!isPaste) return;

  // Fix folder information
  TXsheet *xsh = TApp::instance()->getCurrentXsheet()->getXsheet();
  int c        = *indices.begin() - 1;
  if (c >= 0) {  // column just below insertion point
    TXshColumn *column    = xsh->getColumn(c);
    QStack<int> folderIds = column ? column->getFolderIdStack() : QStack<int>();
    int subfolder         = -1;
    std::set<int>::reverse_iterator rit;
    for (rit = indices.rbegin(); rit != indices.rend(); rit++) {
      int col          = *rit;
      TXshColumn *item = xsh->getColumn(col);
      if (!item) continue;

      if (subfolder >= 0 && !item->isContainedInFolder(subfolder)) {
        folderIds.pop();
        if (folderIds.size())
          subfolder = folderIds.top();
        else
          subfolder = -1;
      }

      item->setFolderIdStack(folderIds);

      TXshFolderColumn *folder = item->getFolderColumn();
      if (folder) {
        subfolder = item->getFolderColumn()->getFolderColumnFolderId();

        // Always assign a new folder id when pasting
        int newFolderId = xsh->getNewFolderId();
        folder->setFolderColumnFolderId(newFolderId);
        folder->setExpanded(true);

        folderIds.push(newFolderId);
      }
    }
  }

  TUndoManager::manager()->add(new PasteColumnsUndo(indices));
  TApp::instance()->getCurrentScene()->setDirtyFlag(true);
}

//*************************************************************************
//    Delete Columns  Command
//*************************************************************************

void ColumnCmd::deleteColumns(std::set<int> &indices, bool onlyColumns,
                              bool withoutUndo) {
  indices.erase(-1);  // Ignore camera column
  if (indices.empty()) return;

  if (!withoutUndo)
    TUndoManager::manager()->add(new DeleteColumnsUndo(indices, onlyColumns));

  deleteColumnsWithoutUndo(&indices, onlyColumns);
  TApp::instance()->getCurrentScene()->setDirtyFlag(true);
}

//=============================================================================
// deleteColumn
//=============================================================================

void ColumnCmd::deleteColumn(int index, bool onlyColumns) {
  std::set<int> ii;
  ii.insert(index);
  ColumnCmd::deleteColumns(ii, onlyColumns, false);
}

//=============================================================================
// cutColumns
//-----------------------------------------------------------------------------

static void cutColumnsWithoutUndo(std::set<int> *indices) {
  copyColumns_internal(*indices);
  deleteColumnsWithoutUndo(indices);
}

void ColumnCmd::cutColumns(std::set<int> &indices) {
  if (indices.empty()) return;

  TUndoManager::manager()->add(new DeleteColumnsUndo(indices, false));

  cutColumnsWithoutUndo(&indices);
  TApp::instance()->getCurrentScene()->setDirtyFlag(true);
}

//=============================================================================
// Undo Resequence
//=============================================================================

class ResequenceUndo final : public TUndo {
  int m_index;
  int m_r0;
  std::vector<TFrameId> m_oldFrames;
  int m_newFramesCount;

public:
  ResequenceUndo(int col, int count)
      : m_index(col), m_r0(0), m_newFramesCount(count) {
    TXsheet *xsh = TApp::instance()->getCurrentXsheet()->getXsheet();
    int r0, r1;
    xsh->getCellRange(col, r0, r1);
    m_r0 = r0;
    assert(r0 <= r1);
    if (r0 > r1) return;
    for (int r = r0; r <= r1; r++) {
      TXshCell cell = xsh->getCell(r, col);
      assert(cell.isEmpty() || cell.m_level->getChildLevel());
      m_oldFrames.push_back(cell.m_frameId);
    }
  }

  void undo() const override {
    TXsheet *xsh = TApp::instance()->getCurrentXsheet()->getXsheet();
    int r0, r1;
    xsh->getCellRange(m_index, r0, r1);
    assert(r0 <= r1);
    if (r0 > r1) return;
    TXshCell cell = xsh->getCell(r0, m_index);
    assert(!cell.isEmpty());
    assert(cell.m_level->getChildLevel());
    xsh->clearCells(r0, m_index, r1 - r0 + 1);
    for (int i = 0; i < (int)m_oldFrames.size(); i++) {
      TFrameId fid = m_oldFrames[i];
      if (fid != TFrameId::EMPTY_FRAME) {
        cell.m_frameId = fid;
        xsh->setCell(m_r0 + i, m_index, cell);
      }
    }
    TApp::instance()->getCurrentXsheet()->notifyXsheetChanged();
  }

  void redo() const override {
    TXsheet *xsh = TApp::instance()->getCurrentXsheet()->getXsheet();
    int r0, r1;
    xsh->getCellRange(m_index, r0, r1);
    assert(r0 <= r1);
    if (r0 > r1) return;
    TXshCell cell = xsh->getCell(r0, m_index);
    assert(!cell.isEmpty());
    assert(cell.m_level->getChildLevel());
    xsh->clearCells(r0, m_index, r1 - r0 + 1);
    for (int i = 0; i < m_newFramesCount; i++) {
      cell.m_frameId = TFrameId(i + 1);
      xsh->setCell(m_r0 + i, m_index, cell);
    }
    TApp::instance()->getCurrentXsheet()->notifyXsheetChanged();
  }

  int getSize() const override {
    return sizeof(*this) + m_oldFrames.size() * sizeof(TFrameId);
  }

  QString getHistoryString() override {
    return QObject::tr("Resequence :  Col%1").arg(QString::number(m_index + 1));
  }

  int getHistoryType() override { return HistoryType::Xsheet; }
};

//=============================================================================
// Resequence
//=============================================================================

bool ColumnCmd::canResequence(int index) {
  TXsheet *xsh       = TApp::instance()->getCurrentXsheet()->getXsheet();
  TXshColumn *column = xsh->getColumn(index);
  if (!column) return false;
  TXshLevelColumn *lcolumn = column->getLevelColumn();
  if (!lcolumn) return false;
  int r0 = 0, r1 = -1;
  if (lcolumn->getRange(r0, r1) == 0) return false;
  assert(r0 <= r1);
  TXshCell cell = lcolumn->getCell(r0);
  assert(!cell.isEmpty());
  TXshLevel *xl = cell.m_level->getChildLevel();
  if (!xl) return false;
  for (int r = r0 + 1; r <= r1; r++) {
    cell = lcolumn->getCell(r);
    if (cell.isEmpty()) continue;
    if (cell.m_level.getPointer() != xl) return false;
  }
  return true;
}

//-----------------------------------------------------------------------------

void ColumnCmd::resequence(int index) {
  if (!canResequence(index)) return;
  TXsheet *xsh       = TApp::instance()->getCurrentXsheet()->getXsheet();
  TXshColumn *column = xsh->getColumn(index);
  assert(column);
  TXshLevelColumn *lcolumn = column->getLevelColumn();
  assert(lcolumn);
  int r0 = 0, r1 = -1;
  lcolumn->getRange(r0, r1);
  assert(r0 <= r1);
  TXshCell cell = lcolumn->getCell(r0);
  assert(!cell.isEmpty());
  TXshChildLevel *xl = cell.m_level->getChildLevel();
  assert(xl);
  TXsheet *childXsh = xl->getXsheet();
  int frameCount    = childXsh->getFrameCount();
  if (frameCount < 1) frameCount = 1;

  TUndoManager::manager()->add(new ResequenceUndo(index, frameCount));

  lcolumn->clearCells(r0, r1 - r0 + 1);
  for (int i = 0; i < frameCount; i++) {
    cell.m_frameId = TFrameId(i + 1);
    lcolumn->setCell(r0 + i, cell);
  }

  xsh->updateFrameCount();

  TApp::instance()->getCurrentScene()->setDirtyFlag(true);
  TApp::instance()->getCurrentXsheet()->notifyXsheetChanged();
}

//=============================================================================
// Undo cloneChild
//=============================================================================

class CloneChildUndo final : public TUndo {
  TXshChildLevelP m_childLevel;
  int m_columnIndex;

public:
  CloneChildUndo(TXshChildLevel *childLevel, int columnIndex)
      : m_childLevel(childLevel), m_columnIndex(columnIndex) {}

  void undo() const override {
    TXsheet *xsh = TApp::instance()->getCurrentXsheet()->getXsheet();
    xsh->removeColumn(m_columnIndex);
    TApp::instance()->getCurrentXsheet()->notifyXsheetChanged();
  }

  void redo() const override {
    TXsheet *xsh = TApp::instance()->getCurrentXsheet()->getXsheet();
    xsh->insertColumn(m_columnIndex);
    int frameCount = m_childLevel->getXsheet()->getFrameCount();
    if (frameCount < 1) frameCount = 1;
    for (int r = 0; r < frameCount; r++)
      xsh->setCell(r, m_columnIndex,
                   TXshCell(m_childLevel.getPointer(), TFrameId(r + 1)));
    TApp::instance()->getCurrentXsheet()->notifyXsheetChanged();
  }

  int getSize() const override {
    // bisognerebbe tener conto della dimensione del sottoxsheet
    return sizeof(*this) + 100;
  }

  QString getHistoryString() override {
    return QObject::tr("Clone Sub-xsheet :  Col%1")
        .arg(QString::number(m_columnIndex + 1));
  }

  int getHistoryType() override { return HistoryType::Xsheet; }
};

//=============================================================================
// cloneChild
//=============================================================================

void ColumnCmd::cloneChild(int index) {
  if (!canResequence(index)) return;

  /*-- カラムを取得 --*/
  TXsheet *xsh       = TApp::instance()->getCurrentXsheet()->getXsheet();
  TXshColumn *column = xsh->getColumn(index);
  assert(column);

  // get the subxsheet to clone (childLevel, childXsh)
  /*-- SubXsheetレベルを取得 --*/
  TXshLevelColumn *lcolumn = column->getLevelColumn();
  assert(lcolumn);
  int r0 = 0, r1 = -1;
  lcolumn->getRange(r0, r1);
  assert(r0 <= r1);
  /*-- SubXsheetの一番頭のセル --*/
  TXshCell cell = lcolumn->getCell(r0);
  assert(!cell.isEmpty());
  /*- cell内に格納されているLevelを取得 -*/
  TXshChildLevel *childLevel = cell.m_level->getChildLevel();
  assert(childLevel);
  /*- SubXsheetのXsheetを取得 -*/
  TXsheet *childXsh = childLevel->getXsheet();

  // insert a new empty column
  /*- 隣に空きColumnをInsertしてCloneに備える -*/
  int newColumnIndex = index + 1;
  xsh->insertColumn(newColumnIndex);

  // create a subxsheet (newChildLevel, newChildXsh)
  ToonzScene *scene      = TApp::instance()->getCurrentScene()->getScene();
  ChildStack *childStack = scene->getChildStack();
  TXshChildLevel *newChildLevel = childStack->createChild(0, newColumnIndex);
  TXsheet *newChildXsh          = newChildLevel->getXsheet();

  // copy columns.
  std::set<int> indices;
  for (int i = 0; i < childXsh->getColumnCount(); i++) indices.insert(i);
  StageObjectsData *data = new StageObjectsData();
  data->storeColumns(indices, childXsh, 0);
  data->storeColumnFxs(indices, childXsh, 0);
  std::list<int> restoredSplineIds;
  QMap<TStageObjectId, TStageObjectId> idTable;
  QMap<TFx *, TFx *> fxTable;
  data->restoreObjects(indices, restoredSplineIds, newChildXsh,
                       StageObjectsData::eDoClone, idTable, fxTable);
  delete data;

  cloneNotColumnLinkedFxsAndOutputsFx(childXsh, newChildXsh, &fxTable);
  cloneXsheetTStageObjectTree(childXsh, newChildXsh);
  /*--以下は、Clone SubXsheet
  するときに、SubXsheet内にある子SubXsheetをクローンする関数
  クローンされた中にある子SubXsheetは、同じもので良いので、スキップする --*/
  // cloneSubXsheets(newChildXsh);

  /*-- XSheetノードのFxSchematicでのDagNodePosを再現
  FxやColumnノードの位置の再現は上のsetColumnで行っている
--*/
  newChildXsh->getFxDag()->getXsheetFx()->getAttributes()->setDagNodePos(
      childXsh->getFxDag()->getXsheetFx()->getAttributes()->getDagNodePos());

  ExpressionReferenceManager::instance()->refreshXsheetRefInfo(newChildXsh);
  ExpressionReferenceManager::instance()->transferReference(
      childXsh, newChildXsh, idTable, fxTable);

  newChildXsh->updateFrameCount();

  /*-- TXshChildLevel作成時にsetCellした1つ目のセルを消去 --*/
  xsh->removeCells(0, newColumnIndex);
  /*-- CloneしたColumnのセル番号順を再現 --*/
  for (int r = r0; r <= r1; r++) {
    TXshCell cell = lcolumn->getCell(r);
    if (cell.isEmpty()) continue;

    cell.m_level = newChildLevel;
    xsh->setCell(r, newColumnIndex, cell);
  }

  TStageObjectId currentObjectId =
      TApp::instance()->getCurrentObject()->getObjectId();
  xsh->getStageObject(TStageObjectId::ColumnId(newColumnIndex))
      ->setParent(xsh->getStageObjectParent(currentObjectId));

  xsh->updateFrameCount();
  TUndoManager::manager()->add(
      new CloneChildUndo(newChildLevel, newColumnIndex));

  // notify changes
  TApp::instance()->getCurrentScene()->setDirtyFlag(true);
  TApp::instance()->getCurrentXsheet()->notifyXsheetChanged();
}

//=============================================================================
// copyColumn
//=============================================================================

void ColumnCmd::copyColumn(int dstIndex, int srcIndex) {
  TXsheet *xsh = TApp::instance()->getCurrentXsheet()->getXsheet();

  // columns[srcIndex] => data
  StageObjectsData *data = new StageObjectsData();
  std::set<int> ii;
  ii.insert(srcIndex);
  data->storeColumns(ii, xsh, StageObjectsData::eDoClone);
  data->storeColumnFxs(ii, xsh, StageObjectsData::eDoClone);

  // data => columns[dstIndex]
  ii.clear();
  ii.insert(dstIndex);
  ColumnCmd::pasteColumns(ii, data);

  delete data;
}

//=============================================================================
// Undo Clear Cells
//=============================================================================

class ClearColumnCellsUndo final : public TUndo {
  int m_col;
  int m_r0;
  std::vector<TXshCell> m_oldFrames;

public:
  ClearColumnCellsUndo(int col) : m_col(col), m_r0(0) {
    TXsheet *xsh = TApp::instance()->getCurrentXsheet()->getXsheet();
    int r0, r1;
    xsh->getCellRange(col, r0, r1);
    m_r0 = r0;
    if (r0 > r1) return;
    for (int r = r0; r <= r1; r++) {
      m_oldFrames.push_back(xsh->getCell(r, col));
    }
  }

  void undo() const override {
    TXsheet *xsh = TApp::instance()->getCurrentXsheet()->getXsheet();
    for (int i = 0; i < (int)m_oldFrames.size(); i++) {
      xsh->setCell(m_r0 + i, m_col, m_oldFrames[i]);
    }
    xsh->updateFrameCount();
    TApp::instance()->getCurrentXsheet()->notifyXsheetChanged();
  }
  void redo() const override {
    TXsheet *xsh = TApp::instance()->getCurrentXsheet()->getXsheet();
    int r0, r1;
    xsh->getCellRange(m_col, r0, r1);
    if (r0 <= r1) {
      xsh->clearCells(r0, m_col, r1 - r0 + 1);
      xsh->updateFrameCount();
      TApp::instance()->getCurrentXsheet()->notifyXsheetChanged();
    }
  }

  int getSize() const override {
    return sizeof(*this) + m_oldFrames.size() * sizeof(m_oldFrames[0]);
  }

  QString getHistoryString() override {
    return QObject::tr("Clear Cells :  Col%1").arg(QString::number(m_col + 1));
  }

  int getHistoryType() override { return HistoryType::Xsheet; }
};

//=============================================================================
// clearCells
//=============================================================================

void ColumnCmd::clearCells(int index) {
  ClearColumnCellsUndo *undo = new ClearColumnCellsUndo(index);
  undo->redo();
  TUndoManager::manager()->add(undo);
  TApp::instance()->getCurrentScene()->setDirtyFlag(true);
}

//=============================================================================
// checkExpressionReferences
//=============================================================================
// - If onlyColumns is true, it means that only columns with specified indices
// will be removed.
// - If onlyColumns is false, it means that the relevant pegbars will be removed
// as well (when collapsing columns).
// - Note that relevant Fxs will be removed / collapsed regardless of
// onlyColumns.
// - When checkInvert is true, check references both side from the main xsheet
// and the child xsheet when collapsing columns.

bool ColumnCmd::checkExpressionReferences(const std::set<int> &indices,
                                          bool onlyColumns, bool checkInvert) {
  if (!Preferences::instance()->isModifyExpressionOnMovingReferencesEnabled())
    return true;

  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  // check if fxs will be deleted
  QSet<int> colIdsToBeDeleted;
  QSet<TFx *> fxsToBeDeleted;
  // store column fxs to be deleted
  std::set<TFx *> leaves;
  for (auto index : indices) {
    if (index < 0) continue;
    TXshColumn *column = xsh->getColumn(index);
    if (!column) continue;
    colIdsToBeDeleted.insert(index);
    TFx *fx = column->getFx();
    if (fx) {
      leaves.insert(fx);
      TZeraryColumnFx *zcfx = dynamic_cast<TZeraryColumnFx *>(fx);
      if (zcfx) fxsToBeDeleted.insert(zcfx->getZeraryFx());
    }
  }

  // store relevant fxs which will be deleted along with the columns
  TFxSet *fxSet = xsh->getFxDag()->getInternalFxs();
  for (int i = 0; i < fxSet->getFxCount(); i++) {
    TFx *fx = fxSet->getFx(i);
    if (canRemoveFx(leaves, fx)) fxsToBeDeleted.insert(fx);
  }

  // store object ids which will be duplicated in the child xsheet on collapse
  QList<TStageObjectId> objIdsToBeDuplicated;
  if (checkInvert && !onlyColumns) {
    for (auto index : indices) {
      TStageObjectId id =
          xsh->getStageObjectParent(TStageObjectId::ColumnId(index));
      // store pegbars/cameras connected to the columns
      while (id.isPegbar() || id.isCamera()) {
        if (!objIdsToBeDuplicated.contains(id)) objIdsToBeDuplicated.append(id);
        id = xsh->getStageObjectParent(id);
      }
    }
  }

  return ExpressionReferenceManager::instance()->checkReferenceDeletion(
      colIdsToBeDeleted, fxsToBeDeleted, objIdsToBeDuplicated, checkInvert);
}

//-----------------------------------------------------------------------------

bool ColumnCmd::checkExpressionReferences(const std::set<int> &indices,
                                          const std::set<TFx *> &fxs,
                                          bool onlyColumns, bool checkInvert) {
  if (!Preferences::instance()->isModifyExpressionOnMovingReferencesEnabled())
    return true;

  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  // check if fxs will be deleted
  QSet<int> colIdsToBeDeleted;
  QSet<TFx *> fxsToBeDeleted;
  for (auto index : indices) {
    if (index < 0) continue;
    TXshColumn *column = xsh->getColumn(index);
    if (!column) continue;
    colIdsToBeDeleted.insert(index);
    TFx *fx = column->getFx();
    if (fx) {
      TZeraryColumnFx *zcfx = dynamic_cast<TZeraryColumnFx *>(fx);
      if (zcfx) fxsToBeDeleted.insert(zcfx->getZeraryFx());
    }
  }

  TFxSet *fxSet = xsh->getFxDag()->getInternalFxs();
  for (auto fx : fxs) fxsToBeDeleted.insert(fx);

  // store object ids which will be duplicated in the child xsheet on collapse
  QList<TStageObjectId> objIdsToBeDuplicated;
  if (checkInvert && !onlyColumns) {
    for (auto index : indices) {
      TStageObjectId id =
          xsh->getStageObjectParent(TStageObjectId::ColumnId(index));
      // store pegbars/cameras connected to the columns
      while (id.isPegbar() || id.isCamera()) {
        if (!objIdsToBeDuplicated.contains(id)) objIdsToBeDuplicated.append(id);
        id = xsh->getStageObjectParent(id);
      }
    }
  }

  return ExpressionReferenceManager::instance()->checkReferenceDeletion(
      colIdsToBeDeleted, fxsToBeDeleted, objIdsToBeDuplicated, checkInvert);
}

//-----------------------------------------------------------------------------

bool ColumnCmd::checkExpressionReferences(
    const QList<TStageObjectId> &objects) {
  if (!Preferences::instance()->isModifyExpressionOnMovingReferencesEnabled())
    return true;

  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  QSet<int> colIdsToBeDeleted;
  QSet<TFx *> fxsToBeDeleted;
  QList<TStageObjectId> objIdsToBeDuplicated;

  // store column fxs to be deleted
  std::set<TFx *> leaves;
  for (auto objId : objects) {
    if (objId.isColumn()) {
      int index = objId.getIndex();
      if (index < 0) continue;
      TXshColumn *column = xsh->getColumn(index);
      if (!column) continue;
      colIdsToBeDeleted.insert(index);
      TFx *fx = column->getFx();
      if (fx) {
        leaves.insert(fx);
        TZeraryColumnFx *zcfx = dynamic_cast<TZeraryColumnFx *>(fx);
        if (zcfx) fxsToBeDeleted.insert(zcfx->getZeraryFx());
      }
    } else
      objIdsToBeDuplicated.append(objId);
  }

  // store relevant fxs which will be deleted along with the columns
  TFxSet *fxSet = xsh->getFxDag()->getInternalFxs();
  for (int i = 0; i < fxSet->getFxCount(); i++) {
    TFx *fx = fxSet->getFx(i);
    if (canRemoveFx(leaves, fx)) fxsToBeDeleted.insert(fx);
  }

  return ExpressionReferenceManager::instance()->checkReferenceDeletion(
      colIdsToBeDeleted, fxsToBeDeleted, objIdsToBeDuplicated, true);
}

//---------------------------------------------------------------------------
// Check if any column has visibility toggles with different states and the
// "unify visibility toggles" option is enabled
void ColumnCmd::unifyColumnVisibilityToggles() {
  TApp *app         = TApp::instance();
  ToonzScene *scene = app->getCurrentScene()->getScene();
  QStringList modifiedColumnNames;
  QList<TXsheet *> doneList;
  QStringList parentStack;
  doUnifyColumnVisibilityToggles_Recursive(
      scene->getTopXsheet(), modifiedColumnNames, doneList, parentStack);
  if (!modifiedColumnNames.isEmpty()) {
    DVGui::warning(
        QObject::tr(
            "The visibility toggles of following columns are modified \n"
            "due to \"Unify Preview and Camstand Visibility Toggles\" "
            "preference option : \n  %1")
            .arg(modifiedColumnNames.join("\n  ")));
    app->getCurrentScene()->setDirtyFlag(true);
  }
}

//=============================================================================
// Loop Columns
//=============================================================================

class LoopColumnsUndo final : public ColumnCommandUndo {
  std::set<int> m_indices;

public:
  LoopColumnsUndo(const std::set<int> &indices) : m_indices(indices) {}

  bool isConsistent() const override { return true; }

  void redo() const override;
  void undo() const override;

  int getSize() const override { return sizeof(*this); }

  QString getHistoryString() override { return QObject::tr("Loop Columns"); }

  int getHistoryType() override { return HistoryType::Xsheet; }
};

//------------------------------------------------------

void LoopColumnsUndo::redo() const {
  if (!m_indices.size()) return;

  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  for (auto c : m_indices) {
    TXshColumn *column = xsh->getColumn(c);

    int r0, r1;
    column->getRange(r0, r1);

    column->addLoop(r0, r1);
  }

  app->getCurrentScene()->setDirtyFlag(true);
  app->getCurrentXsheet()->notifyXsheetChanged();
}

//------------------------------------------------------

void LoopColumnsUndo::undo() const {
  if (!m_indices.size()) return;

  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  for (auto c : m_indices) {
    TXshColumn *column = xsh->getColumn(c);

    QList<std::pair<int, int>> loops = column->getLoops();

    for (auto loop : loops) column->removeLoop(loop);
  }

  app->getCurrentScene()->setDirtyFlag(true);
  app->getCurrentXsheet()->notifyXsheetChanged();
}

//------------------------------------------------------

void ColumnCmd::loopColumns(std::set<int> &indices) {
  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  std::set<int> newLoops;

  for (auto c : indices) {
    if (c < 0) continue;
    TXshColumn *column = xsh->getColumn(c);
    if (!column || column->isEmpty() || column->getSoundColumn() ||
        column->getSoundTextColumn() || column->getFolderColumn() ||
        column->hasLoops())
      continue;

    int r0, r1;
    column->getRange(r0, r1);
    if ((r1 - r0 + 1) <= 1) continue;

    newLoops.insert(c);
  }

  if (newLoops.size()) {
    LoopColumnsUndo *undo = new LoopColumnsUndo(newLoops);
    TUndoManager::manager()->add(undo);
    undo->redo();
  }
}

//=============================================================================
// Remove Frame Loops
//=============================================================================

class RemoveColumnLoopsUndo final : public ColumnCommandUndo {
  QMap<int, QList<std::pair<int, int>>> m_columnLoops;

public:
  RemoveColumnLoopsUndo(const std::set<int> &indices) {
    initialize(indices);
  }

  bool isConsistent() const override { return true; }

  void redo() const override;
  void undo() const override;

  int getSize() const override { return sizeof(*this); }

  QString getHistoryString() override {
    return QObject::tr("Remove Column Loops");
  }

  int getHistoryType() override { return HistoryType::Xsheet; }

private:
  void initialize(const std::set<int> &indices);
};

//------------------------------------------------------

void RemoveColumnLoopsUndo::initialize(const std::set<int> &indices) {
  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  m_columnLoops.clear();

  for (auto c : indices) {
    TXshColumn *column = xsh->getColumn(c);

    QList<std::pair<int, int>> loops = column->getLoops();

    m_columnLoops.insert(c, loops);
  }
}

//------------------------------------------------------

void RemoveColumnLoopsUndo::redo() const {
  if (!m_columnLoops.size()) return;

  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  for (auto c : m_columnLoops.keys()) {
    TXshColumn *column = xsh->getColumn(c);
    for (auto loop : m_columnLoops[c]) {
      column->removeLoop(loop);
    }
  }

  app->getCurrentScene()->setDirtyFlag(true);
  app->getCurrentXsheet()->notifyXsheetChanged();
}

//------------------------------------------------------

void RemoveColumnLoopsUndo::undo() const {
  if (!m_columnLoops.size()) return;

  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  for (auto c : m_columnLoops.keys()) {
    TXshColumn *column = xsh->getColumn(c);
    for (auto loop : m_columnLoops[c]) {
      column->addLoop(loop);
    }
  }

  app->getCurrentScene()->setDirtyFlag(true);
  app->getCurrentXsheet()->notifyXsheetChanged();
}

//-----------------------------------------------------------------------------

void ColumnCmd::removeColumnLoops(std::set<int> &indices) {
  TApp *app    = TApp::instance();
  TXsheet *xsh = app->getCurrentXsheet()->getXsheet();

  std::set<int> removedLoops;

  for (auto c : indices) {
    if (c < 0) continue;
    TXshColumn *column = xsh->getColumn(c);
    if (!column || !column->hasLoops()) continue;

    int r0, r1;
    column->getRange(r0, r1);
    if ((r1 - r0 + 1) <= 1) continue;

    removedLoops.insert(c);
  }

  if (removedLoops.size()) {
    RemoveColumnLoopsUndo *undo = new RemoveColumnLoopsUndo(removedLoops);
    TUndoManager::manager()->add(undo);
    undo->redo();
  }
}

//=============================================================================

namespace {

enum {
  CMD_LOCK        = 0x0001,
  CMD_UNLOCK      = 0x0002,
  CMD_TOGGLE_LOCK = 0x0004,

  CMD_ENABLE_PREVIEW  = 0x0008,
  CMD_DISABLE_PREVIEW = 0x0010,
  CMD_TOGGLE_PREVIEW  = 0x0020,

  CMD_ENABLE_CAMSTAND  = 0x0040,
  CMD_DISABLE_CAMSTAND = 0x0080,
  CMD_TOGGLE_CAMSTAND  = 0x0100
};

enum {
  TARGET_ALL,
  TARGET_SELECTED,
  TARGET_CURRENT,
  TARGET_OTHER,
  TARGET_UPPER /*-- カレントカラムより右側にあるカラムを非表示にするコマンド
                  --*/
};

}  // namespace

class ColumnsStatusCommand final : public MenuItemHandler {
  int m_cmd, m_target;

public:
  ColumnsStatusCommand(CommandId id, int cmd, int target)
      : MenuItemHandler(id), m_cmd(cmd), m_target(target) {}

  void execute() override {
    TColumnSelection *selection = dynamic_cast<TColumnSelection *>(
        TApp::instance()->getCurrentSelection()->getSelection());
    TXsheet *xsh          = TApp::instance()->getCurrentXsheet()->getXsheet();
    XsheetViewer *xviewer = TApp::instance()->getCurrentXsheetViewer();
    int mc                = xviewer->getMenuColumnTarget();
    int cc =
        mc >= -1 ? mc : TApp::instance()->getCurrentColumn()->getColumnIndex();
    bool sound_changed    = false;
    TTool *tool           = TApp::instance()->getCurrentTool()->getTool();
    TTool::Viewer *viewer = tool ? tool->getViewer() : nullptr;
    bool viewer_changed   = false;

    int startCol =
        Preferences::instance()->isXsheetCameraColumnVisible() ? -1 : 0;

    for (int i = startCol; i < xsh->getColumnCount(); i++) {
      /*- Skip if empty column -*/
      if (i >= 0 && xsh->isColumnEmpty(i)) continue;
      /*- Skip if column cannot be obtained -*/
      TXshColumn *column = xsh->getColumn(i);
      if (!column) continue;
      /*- Skip if target is in selected column mode and not selected -*/
      bool isSelected = selection && !selection->isEmpty()
                      ? selection->isColumnSelected(i)
                      : cc == i;
      if (m_target == TARGET_SELECTED && !isSelected) continue;

      /*-
       * Skip if target is "right side of current column" mode and i is left of
       * current column
       * -*/
      if (m_target == TARGET_UPPER && i < cc) continue;

      bool negate = ((m_target == TARGET_CURRENT && cc != i) ||
                     (m_target == TARGET_OTHER && cc == i) ||
                     (m_target == TARGET_UPPER && cc == i));

      int cmd = m_cmd;

      if (cmd & (CMD_LOCK | CMD_UNLOCK | CMD_TOGGLE_LOCK)) {
        if (cmd & CMD_LOCK)
          column->lock(!negate);
        else if (cmd & CMD_UNLOCK)
          column->lock(negate);
        else
          column->lock(!column->isLocked());
        viewer_changed = true;
      }
      if (cmd &
          (CMD_ENABLE_PREVIEW | CMD_DISABLE_PREVIEW | CMD_TOGGLE_PREVIEW)) {
        if (cmd & CMD_ENABLE_PREVIEW)
          column->setPreviewVisible(!negate);
        else if (cmd & CMD_DISABLE_PREVIEW)
          column->setPreviewVisible(negate);
        else
          column->setPreviewVisible(!column->isPreviewVisible());

        // sync camstand visibility
        if (Preferences::instance()->isUnifyColumnVisibilityTogglesEnabled())
          column->setCamstandVisible(column->isPreviewVisible());
      }
      if (cmd &
          (CMD_ENABLE_CAMSTAND | CMD_DISABLE_CAMSTAND | CMD_TOGGLE_CAMSTAND)) {
        if (cmd & CMD_ENABLE_CAMSTAND)
          column->setCamstandVisible(!negate);
        else if (cmd & CMD_DISABLE_CAMSTAND)
          column->setCamstandVisible(negate);
        else
          column->setCamstandVisible(!column->isCamstandVisible());
        if (column->getSoundColumn()) sound_changed = true;
        viewer_changed = true;
      }
      /*TAB
if(cmd & (CMD_ENABLE_PREVIEW|CMD_DISABLE_PREVIEW|CMD_TOGGLE_PREVIEW))
{ //In Tab preview e cameraStand vanno settati entrambi
if(cmd&CMD_ENABLE_PREVIEW)
{
column->setPreviewVisible(!negate);
column->setCamstandVisible(!negate);
}
else if(cmd&CMD_DISABLE_PREVIEW)
{
column->setPreviewVisible(negate);
column->setCamstandVisible(negate);
}
else
{
column->setPreviewVisible(!column->isPreviewVisible());
column->setCamstandVisible(!column->isCamstandVisible());
}
}
      */
    }
    if (sound_changed)
      TApp::instance()->getCurrentXsheet()->notifyXsheetSoundChanged();
    TApp::instance()->getCurrentXsheet()->notifyXsheetChanged();
    TApp::instance()->getCurrentScene()->setDirtyFlag(true);
    if (viewer && viewer_changed) viewer->invalidateToolStatus();
  }
};

namespace {
ColumnsStatusCommand

    c00(MI_ActivateAllColumns, CMD_ENABLE_CAMSTAND, TARGET_ALL),
    c01(MI_DeactivateAllColumns, CMD_DISABLE_CAMSTAND, TARGET_ALL),
    c02(MI_ActivateThisColumnOnly, CMD_ENABLE_CAMSTAND, TARGET_CURRENT),
    c03(MI_ToggleColumnsActivation, CMD_TOGGLE_CAMSTAND, TARGET_ALL),
    c04(MI_ActivateSelectedColumns, CMD_ENABLE_CAMSTAND, TARGET_SELECTED),
    c05(MI_DeactivateSelectedColumns, CMD_DISABLE_CAMSTAND, TARGET_SELECTED),
    c18(MI_DeactivateUpperColumns, CMD_DISABLE_CAMSTAND, TARGET_UPPER),

    c06(MI_EnableAllColumns, CMD_ENABLE_PREVIEW, TARGET_ALL),
    c07(MI_DisableAllColumns, CMD_DISABLE_PREVIEW, TARGET_ALL),
    c08(MI_EnableThisColumnOnly, CMD_ENABLE_PREVIEW, TARGET_CURRENT),
    c09(MI_SwapEnabledColumns, CMD_TOGGLE_PREVIEW, TARGET_ALL),
    c10(MI_EnableSelectedColumns, CMD_ENABLE_PREVIEW, TARGET_SELECTED),
    c11(MI_DisableSelectedColumns, CMD_DISABLE_PREVIEW, TARGET_SELECTED),

    c12(MI_LockAllColumns, CMD_LOCK, TARGET_ALL),
    c13(MI_UnlockAllColumns, CMD_UNLOCK, TARGET_ALL),
    c14(MI_LockThisColumnOnly, CMD_LOCK, TARGET_CURRENT),
    c15(MI_ToggleColumnLocks, CMD_TOGGLE_LOCK, TARGET_ALL),
    c16(MI_LockSelectedColumns, CMD_LOCK, TARGET_SELECTED),
    c17(MI_UnlockSelectedColumns, CMD_UNLOCK, TARGET_SELECTED);
}  // namespace

//=============================================================================
// ConvertToVectorUndo
//-----------------------------------------------------------------------------

// Same in functionality to PasteColumnsUndo; think of it perhaps like
// pasting the newly created vector column.
class ConvertToVectorUndo final : public PasteColumnsUndo {
public:
  ConvertToVectorUndo(std::set<int> indices) : PasteColumnsUndo(indices){};

  QString getHistoryString() override {
    return QObject::tr("Convert to Vectors");
  }
};

void ColumnCmd::addConvertToVectorUndo(std::set<int> &newColumnIndices) {
  TUndoManager::manager()->add(new ConvertToVectorUndo(newColumnIndices));
}

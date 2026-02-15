

#include "tools/tool.h"
#include "tools/toolutils.h"
#include "tthreadmessage.h"
#include "tgl.h"
#include "tstroke.h"
#include "tvectorimage.h"
#include "tmathutil.h"
#include "tools/cursors.h"
#include "tproperty.h"
#include "symmetrytool.h"
#include "symmetrystroke.h"

#include "toonzqt/imageutils.h"

#include "toonz/tframehandle.h"
#include "toonz/tcolumnhandle.h"
#include "toonz/txshlevelhandle.h"
#include "toonz/tobjecthandle.h"
#include "toonz/txsheethandle.h"
#include "toonz/tstageobject.h"
#include "tools/toolhandle.h"
#include "toonz/stage2.h"
#include "tenv.h"
#include "tinbetween.h"



// -----added by TomDoingArt - Freehand mode--------
#include "vectorbrush.h"
#include "tproperty.h"
#include "drawutil.h"

#include "../common/tvectorimage/tvectorimageP.h"

//#include "tapeenv.h"

// ------------------------------------------------



// For Qt translation support
#include <QCoreApplication>

bool debug_mode = true;  // Set to false to disable debug output
#define DEBUG_LOG(x) if (debug_mode) std::cout << x // << std::endl

using namespace ToolUtils;

#define POINT2POINT L"Endpoint to Endpoint"
#define POINT2LINE L"Endpoint to Line"
#define LINE2LINE L"Line to Line"
#define NORMAL L"Normal"
#define RECT L"Rectangular"
#define FREEHAND L"Freehand"
#define LINEAR_INTERPOLATION L"Linear"
#define EASE_IN_INTERPOLATION L"Ease In"
#define EASE_OUT_INTERPOLATION L"Ease Out"
#define EASE_IN_OUT_INTERPOLATION L"Ease In/Out"

TEnv::StringVar TapeMode("InknpaintTapeMode1", "Endpoint to Endpoint");
TEnv::IntVar TapeSmooth("InknpaintTapeSmooth", 0);
TEnv::IntVar TapeJoinStrokes("InknpaintTapeJoinStrokes", 0);
TEnv::StringVar TapeType("InknpaintTapeType1", "Normal");
TEnv::DoubleVar AutocloseFactorMin("InknpaintAutocloseFactorMin", 1.15);
TEnv::DoubleVar AutocloseFactor("InknpaintAutocloseFactor", 4.0);
TEnv::IntVar TapeRange("InknpaintTapeRange", 0);

TEnv::DoubleVar TapeDehookFactorMin("InknpaintTapeDehookMin", 0.01);
TEnv::DoubleVar TapeDehookFactorMax("InknpaintTapeDehookMax", 0.20);
TEnv::DoubleVar TapeDehookAngleThreshold("InknpaintTapeDehookAngleThreshold", 30.00);
TEnv::DoubleVar LineExtensionAngle("InknpaintTapeLineExtensionAngle", 0.30);

namespace {

class UndoAutoclose final : public ToolUtils::TToolUndo {
  int m_oldStrokeId1;
  int m_oldStrokeId2;
  int m_pos1, m_pos2;

  VIStroke *m_oldStroke1;
  VIStroke *m_oldStroke2;

  std::vector<TFilledRegionInf> *m_fillInformation;

  int m_row;
  int m_column;
  std::vector<int> m_changedStrokes;

public:
  VIStroke *m_newStroke;
  int m_newStrokeId;
  int m_newStrokePos;

  UndoAutoclose(TXshSimpleLevel *level, const TFrameId &frameId, int pos1,
                int pos2, std::vector<TFilledRegionInf> *fillInformation,
                const std::vector<int> &changedStrokes)
      : ToolUtils::TToolUndo(level, frameId)
      , m_oldStroke1(0)
      , m_oldStroke2(0)
      , m_pos1(pos1)
      , m_pos2(pos2)
      , m_newStrokePos(-1)
      , m_fillInformation(fillInformation)
      , m_changedStrokes(changedStrokes) {
    TVectorImageP image = level->getFrame(m_frameId, true);
    if (pos1 != -1) {
      m_oldStrokeId1 = image->getStroke(pos1)->getId();
      m_oldStroke1   = cloneVIStroke(image->getVIStroke(pos1));
    }
    if (pos2 != -1 && pos1 != pos2 && image) {
      m_oldStrokeId2 = image->getStroke(pos2)->getId();
      m_oldStroke2   = cloneVIStroke(image->getVIStroke(pos2));
    }
    TTool::Application *app = TTool::getApplication();
    if (app) {
      m_row    = app->getCurrentFrame()->getFrame();
      m_column = app->getCurrentColumn()->getColumnIndex();
    }
  }

  ~UndoAutoclose() {
    deleteVIStroke(m_newStroke);
    if (m_oldStroke1) deleteVIStroke(m_oldStroke1);
    if (m_oldStroke2) deleteVIStroke(m_oldStroke2);
    if (m_isLastInBlock) delete m_fillInformation;
  }

  void undo() const override {
    TTool::Application *app = TTool::getApplication();
    if (!app) return;
    DEBUG_LOG("\nUndoAutoclose.undo() was called.\n");
    if (app->getCurrentFrame()->isEditingScene()) {
      app->getCurrentColumn()->setColumnIndex(m_column);
      app->getCurrentFrame()->setFrame(m_row);
    } else
      app->getCurrentFrame()->setFid(m_frameId);
    TVectorImageP image = m_level->getFrame(m_frameId, true);
    assert(!!image);
    if (!image) return;
    QMutexLocker lock(image->getMutex());
    int strokeIndex = image->getStrokeIndexById(m_newStrokeId);
    if (strokeIndex != -1) image->removeStroke(strokeIndex);

    if (m_oldStroke1)
      image->insertStrokeAt(cloneVIStroke(m_oldStroke1), m_pos1);
    if (m_oldStroke2)
      image->insertStrokeAt(cloneVIStroke(m_oldStroke2), m_pos2);

    image->notifyChangedStrokes(m_changedStrokes, std::vector<TStroke *>());

    if (!m_isLastInBlock) return;

    for (UINT i = 0; i < m_fillInformation->size(); i++) {
      TRegion *reg = image->getRegion((*m_fillInformation)[i].m_regionId);
      assert(reg);
      if (reg) reg->setStyle((*m_fillInformation)[i].m_styleId);
    }
    app->getCurrentXsheet()->notifyXsheetChanged();
    notifyImageChanged();
  }

  void redo() const override {
    TTool::Application *app = TTool::getApplication();
    if (!app) return;
    DEBUG_LOG("\nUndoAutoclose.redo() was called.\n");

    if (app->getCurrentFrame()->isEditingScene()) {
      app->getCurrentColumn()->setColumnIndex(m_column);
      app->getCurrentFrame()->setFrame(m_row);
    } else
      app->getCurrentFrame()->setFid(m_frameId);
    TVectorImageP image = m_level->getFrame(m_frameId, true);
    assert(!!image);
    if (!image) return;
    QMutexLocker lock(image->getMutex());
    if (m_oldStroke1) {
      int strokeIndex = image->getStrokeIndexById(m_oldStrokeId1);
      if (strokeIndex != -1) image->removeStroke(strokeIndex);
    }

    if (m_oldStroke2) {
      int strokeIndex = image->getStrokeIndexById(m_oldStrokeId2);
      if (strokeIndex != -1) image->removeStroke(strokeIndex);
    }

    VIStroke *stroke = cloneVIStroke(m_newStroke);
    image->insertStrokeAt(stroke, m_pos1 == -1 ? m_newStrokePos : m_pos1,
                          false);

    image->notifyChangedStrokes(m_changedStrokes, std::vector<TStroke *>());

    app->getCurrentXsheet()->notifyXsheetChanged();
    notifyImageChanged();
  }

  int getSize() const override {
    return sizeof(*this) +
           m_fillInformation->capacity() * sizeof(TFilledRegionInf) + 500;
  }

  QString getToolName() override { return QString("Autoclose Tool"); }
  int getHistoryType() override { return HistoryType::AutocloseTool; }
};

}  // namespace

/*
namespace {

  class UndoLineExtensionAutoclose final : public ToolUtils::TToolUndo {
    int m_oldStrokeId1;
    int m_oldStrokeId2;
    int m_pos1, m_pos2;

    VIStroke* m_oldStroke1;
    VIStroke* m_oldStroke2;

    std::vector<TFilledRegionInf>* m_fillInformation;

    int m_row;
    int m_column;
    std::vector<int> m_changedStrokes;

  public:
    VIStroke* m_newStroke;
    int m_newStrokeId;
    int m_newStrokePos;
    std::vector<VIStroke*> m_addedStrokes;
    std::vector<int> m_addedStrokeIDs;

    UndoLineExtensionAutoclose(TXshSimpleLevel* level, const TFrameId& frameId) : ToolUtils::TToolUndo(level, frameId) 
    {
      DEBUG_LOG("\nUndoLineExtensionAutoclose(TXshSimpleLevel* level, const TFrameId& frameId)\n");
      TVectorImageP image = level->getFrame(m_frameId, true);
      m_frameId = frameId;
      TTool::Application* app = TTool::getApplication();
      if (app) {
        m_row = app->getCurrentFrame()->getFrame();
        m_column = app->getCurrentColumn()->getColumnIndex();
      }
    }
        
    UndoLineExtensionAutoclose(TXshSimpleLevel* level, const TFrameId& frameId,
      const std::vector<VIStroke*> addedStrokes, std::vector<int> newStrokeIDs)
          : ToolUtils::TToolUndo(level, frameId)
      , m_addedStrokeIDs(newStrokeIDs)
      , m_addedStrokes(addedStrokes){
      DEBUG_LOG("\nUndoLineExtensionAutoclose(TXshSimpleLevel * level, const TFrameId & frameId,const std::vector<VIStroke*> addedStrokes, std::vector<int> newStrokeIDs)\n");
      TVectorImageP image = level->getFrame(m_frameId, true);
      //if (pos1 != -1) {
      //  m_oldStrokeId1 = image->getStroke(pos1)->getId();
      //  m_oldStroke1 = cloneVIStroke(image->getVIStroke(pos1));
      //}
      //if (pos2 != -1 && pos1 != pos2 && image) {
      //  m_oldStrokeId2 = image->getStroke(pos2)->getId();
      //  m_oldStroke2 = cloneVIStroke(image->getVIStroke(pos2));
      //}
      TTool::Application* app = TTool::getApplication();
      if (app) {
        m_row = app->getCurrentFrame()->getFrame();
        m_column = app->getCurrentColumn()->getColumnIndex();
      }
    }

    ~UndoLineExtensionAutoclose() {
      if (true) return;
      //deleteVIStroke(m_newStroke);
      //if (m_oldStroke1) deleteVIStroke(m_oldStroke1);
      //if (m_oldStroke2) deleteVIStroke(m_oldStroke2);
      //if (m_isLastInBlock) delete m_fillInformation;
      // delete stroke collection
      for (VIStroke* currStroke : m_addedStrokes) {
        deleteVIStroke(currStroke);
      }
    }

    void undo() const override {
      DEBUG_LOG("\nUndoLineExtensionAutoclose.undo() was called.\n");
      //if (true) return;
      // delete the strokes added
      TTool::Application* app = TTool::getApplication();
      if (!app) return;
      if (app->getCurrentFrame()->isEditingScene()) {
        app->getCurrentColumn()->setColumnIndex(m_column);
        app->getCurrentFrame()->setFrame(m_row);
      }
      else
        app->getCurrentFrame()->setFid(m_frameId);
      TVectorImageP image = m_level->getFrame(m_frameId, true);
      assert(!!image);
      if (!image) return;
      QMutexLocker lock(image->getMutex());

      DEBUG_LOG("undo has made it to step 1. m_addedStrokeIDs.size():" << m_addedStrokeIDs.size() << "\n");

      for (int currStrokeId : m_addedStrokeIDs) {
        int strokeIndex = image->getStrokeIndexById(currStrokeId);
        if (strokeIndex != -1){
          image->removeStroke(strokeIndex);
          DEBUG_LOG("remove stroke index:" << strokeIndex << "\n");
        }
        //if (m_oldStroke1)
        //  image->insertStrokeAt(cloneVIStroke(m_oldStroke1), m_pos1);
        //if (m_oldStroke2)
        //  image->insertStrokeAt(cloneVIStroke(m_oldStroke2), m_pos2);

      }

      DEBUG_LOG("undo has made it to step 2. m_isLastInBlock:" << m_isLastInBlock <<"\n");
      //image->notifyChangedStrokes(m_changedStrokes, std::vector<TStroke*>());

      if (!m_isLastInBlock) return;

      //for (UINT i = 0; i < m_fillInformation->size(); i++) {
      //  TRegion* reg = image->getRegion((*m_fillInformation)[i].m_regionId);
      //  assert(reg);
      //  if (reg) reg->setStyle((*m_fillInformation)[i].m_styleId);
      //}
      DEBUG_LOG("undo has made it to step 3\n");
      app->getCurrentXsheet()->notifyXsheetChanged();
      DEBUG_LOG("undo has made it to step 4\n");
      notifyImageChanged();
      DEBUG_LOG("undo has made it to step 5\n");
    }


    void redo() const override {
      DEBUG_LOG("\nUndoLineExtensionAutoclose.redo() was called.\n");
      //if (true) return;
      TTool::Application* app = TTool::getApplication();
      if (!app) return;

      if (app->getCurrentFrame()->isEditingScene()) {
        app->getCurrentColumn()->setColumnIndex(m_column);
        app->getCurrentFrame()->setFrame(m_row);
      }
      else
        app->getCurrentFrame()->setFid(m_frameId);

      TVectorImageP image = m_level->getFrame(m_frameId, true);
      assert(!!image);
      if (!image) return;
      QMutexLocker lock(image->getMutex());
      
      //if (m_oldStroke1) {
      //  int strokeIndex = image->getStrokeIndexById(m_oldStrokeId1);
      //  if (strokeIndex != -1) image->removeStroke(strokeIndex);
      //}

      //if (m_oldStroke2) {
      //  int strokeIndex = image->getStrokeIndexById(m_oldStrokeId2);
      //  if (strokeIndex != -1) image->removeStroke(strokeIndex);
      //}

      // iterate through the vector of gap close strokes and add them to the image

      for (VIStroke *currStroke : m_addedStrokes) {
        VIStroke *gapCloseStroke = cloneVIStroke(currStroke);
        TStroke *myStroke        = gapCloseStroke->m_s;
        image->addStroke(myStroke);
      }

      //VIStroke* stroke = cloneVIStroke(m_newStroke);
      //image->insertStrokeAt(stroke, m_pos1 == -1 ? m_newStrokePos : m_pos1,
      //  false);

//      image->notifyChangedStrokes(m_changedStrokes, std::vector<TStroke*>());

      app->getCurrentXsheet()->notifyXsheetChanged();
      notifyImageChanged();
    }

    int getSize() const override {
      return sizeof(*this);
      //return sizeof(*this) +
      //  m_fillInformation->capacity() * sizeof(TFilledRegionInf) + 500;
    }

    QString getToolName() override { return QString("Line Extension Autoclose Tool"); }
    int getHistoryType() override { return HistoryType::AutocloseTool; }
  };

}  // namespace
*/

//=============================================================================
// Autoclose Tool
//-----------------------------------------------------------------------------

class VectorTapeTool final : public TTool {
  Q_DECLARE_TR_FUNCTIONS(VectorTapeTool)

  bool m_secondPoint;
  int m_strokeIndex1, m_strokeIndex2;
  double m_w1, m_w2, m_pixelSize;
  TPointD m_pos;
  bool m_firstTime;
  bool m_leftButtonWasDown;
  TRectD m_selectionRect;
  TPointD m_startRect;

  TBoolProperty m_smooth;
  TBoolProperty m_joinStrokes;
  TEnumProperty m_mode;
  TPropertyGroup m_prop;
  TDoublePairProperty m_autocloseFactor;
  TEnumProperty m_type;
  TEnumProperty m_multi;
  TDoublePairProperty m_dehookFactor;
  TDoubleProperty m_dehookAngleThreshold;
  TDoubleProperty m_lineExtensionAngle;

  SymmetryStroke m_polyline;

  TRectD m_firstRect;
  bool m_firstFrameSelected;
  TFrameId m_firstFrameId, m_veryFirstFrameId;
  int m_firstFrameIdx;
  std::pair<int, int> m_currCell;
  SymmetryStroke m_firstPolyline;
  TXshSimpleLevelP m_level;
  //TStroke m_freehandLasso;
  TStroke* m_firstFreehandLasso;

public:
  VectorTapeTool()
      : TTool("T_Tape")
      , m_secondPoint(false)
      , m_strokeIndex1(-1)
      , m_strokeIndex2(-1)
      , m_w1(-1.0)
      , m_w2(-1.0)
      , m_pixelSize(1)
      , m_lineExtensionAngle("LineExtAngle", 0.05, 0.9, .30) // (label, min, max, default)
      , m_dehookAngleThreshold("DehookAngleThreshold", 10.0, 180.0, 30.00) // (label, min, max, default)
      , m_dehookFactor("Dehook", 0.0, 0.90, 0.0, .30)
      , m_smooth("Smooth", false)  // W_ToolOptions_Smooth
      , m_joinStrokes("JoinStrokes", false)
      , m_mode("Mode")
      , m_type("Type")
      , m_autocloseFactor("Distance", 0.01, 100, 1.15, 4)
      , m_firstTime(true)
      , m_leftButtonWasDown(false)
      , m_selectionRect()
      , m_startRect()
      , m_multi("Frame Range:")
      , m_firstFrameSelected(false)
      , m_currCell(-1, -1)
      , m_level(0) {
    bind(TTool::Vectors);

    m_prop.bind(m_type);
    m_prop.bind(m_mode);
    m_prop.bind(m_multi);
    m_multi.addValue(L"Off");
    m_multi.addValue(LINEAR_INTERPOLATION);
    m_multi.addValue(EASE_IN_INTERPOLATION);
    m_multi.addValue(EASE_OUT_INTERPOLATION);
    m_multi.addValue(EASE_IN_OUT_INTERPOLATION);
    m_prop.bind(m_autocloseFactor);
    m_prop.bind(m_joinStrokes);
    m_prop.bind(m_smooth);

    m_mode.addValue(POINT2POINT);
    m_mode.addValue(POINT2LINE);
    m_mode.addValue(LINE2LINE);
    m_smooth.setId("Smooth");
    m_type.addValue(NORMAL);
    m_type.addValue(RECT);
    m_type.addValue(FREEHAND);

    m_mode.setId("Mode");
    m_type.setId("Type");
    m_joinStrokes.setId("JoinVectors");
    m_autocloseFactor.setId("Distance");
    m_multi.setId("FrameRange");

    m_dehookFactor.setId("Dehook");
    m_prop.bind(m_dehookFactor);

    m_dehookFactor.setId("DehookAngleThreshold");
    m_prop.bind(m_dehookAngleThreshold);

    m_lineExtensionAngle.setId("LineExtAngle");
    m_prop.bind(m_lineExtensionAngle);
    
  }

  //-----------------------------------------------------------------------------

  ToolType getToolType() const override { return TTool::LevelWriteTool; }

  //-----------------------------------------------------------------------------

  //void setGapCheck() {
  //  std::wstring s = m_type.getValue();
  //  if (!s.empty()) {
  //    std::cout << "\tTape type:" << ::to_string(s) << "\n";

  //    ToonzCheck* check = ToonzCheck::instance();

  //    if (s == FREEHAND) {
  //      check->setCheck(ToonzCheck::eLineExtensionGapClose);
  //    }
  //    else {
  //      check->clearCheck(ToonzCheck::eLineExtensionGapClose);
  //    }

  //  }
  //  else {
  //    std::cout << "\tTape type unknown\n";
  //  }
  //}

  //void setGapCheck() {
  //  std::wstring s = m_type.getValue();
  //  if (s.empty()) {
  //    std::cout << "\tTape type unknown\n";
  //    return;
  //  }

  //  std::cout << "\tTape type:" << ::to_string(s) << "\n";

  //  ToonzCheck* check = ToonzCheck::instance();
  //  bool isFreehand = (s == FREEHAND);

  //  // Only update if the state has changed
  //  if (isFreehand != (check->getChecks() & ToonzCheck::eLineExtensionGapClose)) {
  //    if (isFreehand) {
  //      check->setCheck(ToonzCheck::eLineExtensionGapClose);
  //    }
  //    else {
  //      check->clearCheck(ToonzCheck::eLineExtensionGapClose);
  //    }
  //  }
  //}


  void setGapCheck() {
    std::wstring s = m_type.getValue();
    if (s.empty()) {
      std::cout << "\tTape type unknown\n";
      return;
    }

    std::cout << "\tTape type:" << ::to_string(s) << "\n";

    ToonzCheck* check = ToonzCheck::instance();
    bool isFreehand = (s == FREEHAND);
    bool isCurrentlySet = check->isCheckEnabled(ToonzCheck::eLineExtensionGapClose);

    if (isFreehand != isCurrentlySet) {
      if (isFreehand) {
        check->setCheck(ToonzCheck::eLineExtensionGapClose);
      }
      else {
        check->clearCheck(ToonzCheck::eLineExtensionGapClose);
      }
    }
  }



  //-----------------------------------------------------------------------------

  //const TEnumProperty& getTypeProperty() const {
  //  return m_type;
  //}
  
  //-----------------------------------------------------------------------------

  bool onPropertyChanged(std::string propertyName) override {
    //DEBUG_LOG("onPropertyChanged() for:" << propertyName << "\n");
    TapeMode       = ::to_string(m_mode.getValue());
    TapeSmooth     = (int)(m_smooth.getValue());
    TapeRange      = m_multi.getIndex();
    std::wstring s = m_type.getValue();
    if (!s.empty()) TapeType = ::to_string(s);
    TapeJoinStrokes = (int)(m_joinStrokes.getValue());
    AutocloseFactorMin = (double)(m_autocloseFactor.getValue().first);
    AutocloseFactor = (double)(m_autocloseFactor.getValue().second);
    m_selectionRect = TRectD();
    m_startRect     = TPointD();
    TapeDehookFactorMin = (double)(m_dehookFactor.getValue().first);
    TapeDehookFactorMax = (double)(m_dehookFactor.getValue().second);
    TapeDehookAngleThreshold = (double)(m_dehookAngleThreshold.getValue());
    LineExtensionAngle = (double)(m_lineExtensionAngle.getValue());

    if (propertyName == "Type") {
      setGapCheck();
      return true;
    }

    if (propertyName == "Distance" &&
      (ToonzCheck::instance()->getChecks() & ToonzCheck::eAutoclose)) {
      notifyImageChanged();
      return true;
    }

    if ((propertyName == "Dehook") &&
      (ToonzCheck::instance()->getChecks() & ToonzCheck::eAutoclose)) {
      notifyImageChanged();
      return true;
    }

    if ((propertyName == "DehookAngleThreshold") &&
      (ToonzCheck::instance()->getChecks() & ToonzCheck::eAutoclose)) {
      notifyImageChanged();
      return true;
    }

    if ((propertyName == "LineExtAngle") &&
      (ToonzCheck::instance()->getChecks() & ToonzCheck::eAutoclose)) {
      notifyImageChanged();
      return true;
    }
  }

  //-----------------------------------------------------------------------------
  void updateTranslation() override {
    //m_startAt.setQStringName(tr("Start At"));
    //m_incBy.setQStringName(tr("Inc. By"));
    m_lineExtensionAngle.setQStringName(tr("Line Ext. Angle"));
    m_dehookAngleThreshold.setQStringName(tr("Threshold Angle"));
    m_dehookFactor.setQStringName(tr("Dehook"));

    m_smooth.setQStringName(tr("Smooth"));
    m_joinStrokes.setQStringName(tr("Join Vectors"));
    m_autocloseFactor.setQStringName(tr("Distance"));

    m_mode.setQStringName(tr("Mode:"));
    m_mode.setItemUIName(POINT2POINT, tr("Endpoint to Endpoint"));
    m_mode.setItemUIName(POINT2LINE, tr("Endpoint to Line"));
    m_mode.setItemUIName(LINE2LINE, tr("Line to Line"));

    m_type.setQStringName(tr("Type:"));
    m_type.setItemUIName(NORMAL, tr("Normal"));
    m_type.setItemUIName(RECT, tr("Rectangular"));
    m_type.setItemUIName(FREEHAND, tr("Freehand"));

    m_multi.setQStringName(tr("Frame Range:"));
    m_multi.setItemUIName(L"Off", tr("Off"));
    m_multi.setItemUIName(LINEAR_INTERPOLATION, tr("Linear"));
    m_multi.setItemUIName(EASE_IN_INTERPOLATION, tr("Ease In"));
    m_multi.setItemUIName(EASE_OUT_INTERPOLATION, tr("Ease Out"));
    m_multi.setItemUIName(EASE_IN_OUT_INTERPOLATION, tr("Ease In/Out"));
  }

  //-----------------------------------------------------------------------------

  TPropertyGroup *getProperties(int targetType) override { return &m_prop; }

  void drawConnection(TThickPoint point1, TThickPoint point2) {
    DEBUG_LOG("drawConnection()\n");
    tglColor(TPixelD(0.1, 0.9, 0.1));

    m_pixelSize  = getPixelSize();
    double thick = std::max(6.0 * m_pixelSize, point1.thick);

    tglDrawCircle(point1, thick);

    if (point2 == TThickPoint()) return;

    thick = std::max(6.0 * m_pixelSize, point2.thick);

    tglDrawCircle(point2, thick);
    tglDrawSegment(point1, point2);
  }

  void draw() override {
    int devPixRatio = m_viewer->getDevPixRatio();

    TVectorImageP vi(getImage(false));
    if (!vi) return;
    //DEBUG_LOG("draw()\n");
    glLineWidth(1.0 * devPixRatio);

    // TAffine viewMatrix = getViewer()->getViewMatrix();
    // glPushMatrix();
    // tglMultMatrix(viewMatrix);
    TPixel color = TPixel32::Red;
    if (m_type.getValue() == RECT) {
      if (m_multi.getIndex() && m_firstFrameSelected) {
        if (m_firstPolyline.size() > 1) {
          m_firstPolyline.drawRectangle(color);
        } else
          ToolUtils::drawRect(m_firstRect, color, 0x3F33, true);
      }

      if (!m_selectionRect.isEmpty())
        if (m_polyline.size() > 1) {
          m_polyline.drawRectangle(color);
        } else
          ToolUtils::drawRect(m_selectionRect, color, 0x3F33, true);
      return;
    }

    if (m_type.getValue() == FREEHAND) {
      //if (m_multi.getIndex() && m_firstFrameSelected) {
      //  if (m_firstPolyline.size() > 1) {
      //    m_firstPolyline.drawRectangle(color);
      //  }
      //  else
      //    ToolUtils::drawRect(m_firstRect, color, 0x3F33, true);
      //}
      if (!m_track.isEmpty()) {
        double pixelSize2 = getPixelSize() * getPixelSize();
        m_thick           = sqrt(pixelSize2) / 2.0;
        TPixel color =
            ToonzCheck::instance()->getChecks() & ToonzCheck::eBlackBg
                ? TPixel32::White
                : TPixel32::Red;
        tglColor(color);
        m_track.drawAllFragments();
      }
      //if (!m_selectionRect.isEmpty()){
      //  if (m_polyline.size() > 1) {
      //    m_polyline.drawRectangle(color);
      //  } else {
      //    ToolUtils::drawRect(m_selectionRect, color, 0x3F33, true);
      //  }
      //}
      return;
    }

    if (m_strokeIndex1 == -1 || m_strokeIndex1 >= (int)(vi->getStrokeCount()))
      return;

    TStroke *stroke1   = vi->getStroke(m_strokeIndex1);
    TThickPoint point1 = stroke1->getPoint(m_w1);
    // TThickPoint point1 = stroke1->getControlPoint(m_cpIndex1);
    TThickPoint point2;

    if (m_secondPoint) {
      if (m_strokeIndex2 != -1) {
        TStroke *stroke2 = vi->getStroke(m_strokeIndex2);
        point2           = stroke2->getPoint(m_w2);
      } else {
        point2 = TThickPoint(m_pos, 4 * m_pixelSize);
      }
    }

    drawConnection(point1, point2);

    SymmetryTool *symmetryTool = dynamic_cast<SymmetryTool *>(
        TTool::getTool("T_Symmetry", TTool::RasterImage));
    if (symmetryTool && !symmetryTool->isGuideEnabled()) return;

    TPointD dpiScale       = getViewer()->getDpiScale();

    std::vector<TPointD> firstPts = symmetryTool->getSymmetryPoints(
        point1, TPointD(), getViewer()->getDpiScale());
    std::vector<TPointD> secondPts = symmetryTool->getSymmetryPoints(
        point2, TPointD(), getViewer()->getDpiScale());

    for (int i = 1; i < firstPts.size(); i++) {
      int strokeIndex1 = vi->getStrokeIndexAtPos(firstPts[i], getPixelSize());
      if (strokeIndex1 == -1) continue;
      drawConnection(firstPts[i], secondPts[i]);
    }
    // glPopMatrix();
  }

  //-----------------------------------------------------------------------------

  void findFirstStroke(TPointD pos, TVectorImageP vi, int &strokeIndex1,
                       double &w1) {
    DEBUG_LOG("findFirstStroke()\n");
    double minDistance2 = 10000000000.;

    int i, strokeNumber = vi->getStrokeCount();

    TStroke *stroke;
    double distance2, outW;
    TPointD point;
    int cpMax;

    for (i = 0; i < strokeNumber; i++) {
      stroke = vi->getStroke(i);
      if (m_mode.getValue() == LINE2LINE) {
        if (stroke->getNearestW(pos, outW, distance2) &&
            distance2 < minDistance2) {
          minDistance2 = distance2;
          strokeIndex1 = i;
          if (areAlmostEqual(outW, 0.0, 1e-3))
            w1 = 0.0;
          else if (areAlmostEqual(outW, 1.0, 1e-3))
            w1 = 1.0;
          else
            w1 = outW;
        }
      } else if (!stroke->isSelfLoop()) {
        point = stroke->getControlPoint(0);
        if ((distance2 = tdistance2(pos, point)) < minDistance2) {
          minDistance2 = distance2;
          strokeIndex1 = i;
          w1           = 0.0;
        }

        cpMax = stroke->getControlPointCount() - 1;
        point = stroke->getControlPoint(cpMax);
        if ((distance2 = tdistance2(pos, point)) < minDistance2) {
          minDistance2 = distance2;
          strokeIndex1 = i;
          w1           = 1.0;
        }
      }
    }
  }

  //-----------------------------------------------------------------------------

  void findSecondStroke(TPointD pos, TVectorImageP vi, int strokeIndex1,
                        double w1, int &strokeIndex, double &w) {
    DEBUG_LOG("findSecondStroke()\n");
    double minDistance2 = 900 * m_pixelSize;

    int i, strokeNumber = vi->getStrokeCount();

    TStroke *stroke;
    double distance2, outW;
    TPointD point;
    int cpMax;

    for (i = 0; i < strokeNumber; i++) {
      if (!vi->sameGroup(strokeIndex1, i) &&
          (vi->isStrokeGrouped(strokeIndex1) || vi->isStrokeGrouped(i)))
        continue;
      stroke = vi->getStroke(i);
      if (m_mode.getValue() != POINT2POINT) {
        if (stroke->getNearestW(pos, outW, distance2) &&
            distance2 < minDistance2) {
          minDistance2 = distance2;
          strokeIndex  = i;
          if (areAlmostEqual(outW, 0.0, 1e-3))
            w = 0.0;
          else if (areAlmostEqual(outW, 1.0, 1e-3))
            w = 1.0;
          else
            w = outW;
        }
      }

      if (!stroke->isSelfLoop()) {
        cpMax = stroke->getControlPointCount() - 1;
        if (!(strokeIndex1 == i && (w1 == 0.0 || cpMax < 3))) {
          point = stroke->getControlPoint(0);
          if ((distance2 = tdistance2(pos, point)) < minDistance2) {
            minDistance2 = distance2;
            strokeIndex  = i;
            w            = 0.0;
          }
        }

        if (!(strokeIndex1 == i && (w1 == 1.0 || cpMax < 3))) {
          point = stroke->getControlPoint(cpMax);
          if ((distance2 = tdistance2(pos, point)) < minDistance2) {
            minDistance2 = distance2;
            strokeIndex  = i;
            w            = 1.0;
          }
        }
      }
    }
  }

  //-----------------------------------------------------------------------------

  void mouseMove(const TPointD &pos, const TMouseEvent &) override {
    TVectorImageP vi(getImage(false));
    if (!vi) return;

    if (m_type.getValue() == RECT) return;

    if (m_type.getValue() == FREEHAND) return;

    m_strokeIndex1 = -1;
    m_secondPoint  = false;

    findFirstStroke(pos, vi, m_strokeIndex1, m_w1);

    invalidate();
  }

  //-----------------------------------------------------------------------------

  VectorBrush m_track;  //!< Lasso selection generator.
  TPointD m_freehand_firstPos;

  double pixelSize2 = getPixelSize() * getPixelSize();
  double m_thick = pixelSize2 / 2.0;
  TStroke* m_stroke;  //!< Stores the stroke generated by m_track.

  //TBoolProperty m_invertOption = TBoolProperty("invert",false);
    
  //! \b pos is added to \b m_track and the first piece of the lasso is drawn.
  //! \b m_firstPos is initialized.
  void startFreehand(const TPointD& pos) {
    m_track.reset();

    SymmetryTool* symmetryTool = dynamic_cast<SymmetryTool*>(
      TTool::getTool("T_Symmetry", TTool::RasterImage));
    TPointD dpiScale = getViewer()->getDpiScale();
    SymmetryObject symmObj = symmetryTool->getSymmetryObject();

    //if (!m_invertOption.getValue() && symmetryTool &&
    if (symmetryTool && symmetryTool->isGuideEnabled()) {
      m_track.addSymmetryBrushes(symmObj.getLines(), symmObj.getRotation(),
        symmObj.getCenterPoint(),
        symmObj.isUsingLineSymmetry(), dpiScale);
    }
    m_freehand_firstPos = pos;
    double pixelSize2 = getPixelSize() * getPixelSize();
    m_track.add(TThickPoint(pos, m_thick), pixelSize2);
  }


  //-----------------------------------------------------------------------------

  //! \b pos is added to \b m_track and another piece of the lasso is drawn.
  void freehandDrag(const TPointD& pos) {
#if defined(MACOSX)
    //		m_viewer->enableRedraw(false);
#endif
    double pixelSize2 = getPixelSize() * getPixelSize();
    m_track.add(TThickPoint(pos, m_thick), pixelSize2);
    invalidate(m_track.getModifiedRegion());
  }

  //-----------------------------------------------------------------------------

  //! The lasso is closed (the last point is added to m_track) and the stroke representing the lasso is created.
  void closeFreehand(const TPointD& pos) {
    DEBUG_LOG("closeFreehand()\n");
#if defined(MACOSX)
    //		m_viewer->enableRedraw(true);
#endif
    if (m_track.isEmpty()) {
      DEBUG_LOG("closeFreehand, m_track is empty.\n");
      return;
    }
    double pixelSize2 = getPixelSize() * getPixelSize();
    m_thick = sqrt(pixelSize2) / 2.0;
    m_track.add(TThickPoint(m_freehand_firstPos, m_thick), pixelSize2);
    m_track.filterPoints();
    double error = (30.0 / 11) * sqrt(pixelSize2);
    m_stroke = m_track.makeStroke(error);
    m_stroke->setStyle(1);
  }

  /*
  // =========================================================================================================

  //! Viene aggiunto \b pos a \b m_track e disegnato il primo pezzetto del
//! lazzo. Viene inizializzato \b m_firstPos
  void startFreehand(const TPointD& pos) {
    m_track.reset();

    SymmetryTool* symmetryTool = dynamic_cast<SymmetryTool*>(
      TTool::getTool("T_Symmetry", TTool::RasterImage));
    TPointD dpiScale = getViewer()->getDpiScale();
    SymmetryObject symmObj = symmetryTool->getSymmetryObject();

    if (symmetryTool && symmetryTool->isGuideEnabled()) {
      m_track.addSymmetryBrushes(symmObj.getLines(), symmObj.getRotation(),
        symmObj.getCenterPoint(),
        symmObj.isUsingLineSymmetry(), dpiScale);
    }

    m_firstPos = pos;
    double pixelSize2 = getPixelSize() * getPixelSize();
    m_track.add(TThickPoint(pos, m_thick), pixelSize2);
  }

  //------------------------------------------------------------------

  //! Viene aggiunto \b pos a \b m_track e disegnato un altro pezzetto del
  //! lazzo.
  void freehandDrag(const TPointD& pos) {
    double pixelSize2 = getPixelSize() * getPixelSize();
    m_track.add(TThickPoint(pos, m_thick), pixelSize2);
  }

  //------------------------------------------------------------------

  //! Viene chiuso il lazzo (si aggiunge l'ultimo punto ad m_track) e viene
  //! creato lo stroke rappresentante il lazzo.
  void closeFreehand(const TPointD& pos) {
    if (m_track.isEmpty()) return;
    double pixelSize2 = getPixelSize() * getPixelSize();
    m_track.add(TThickPoint(m_firstPos, m_thick), pixelSize2);
    m_track.filterPoints();
    double error = (30.0 / 11) * sqrt(pixelSize2);
    m_stroke = m_track.makeStroke(error);
    m_stroke->setStyle(1);
  }

  // =========================================================================================================
  */

  //-----------------------------------------------------------------------------

  void leftButtonDown(const TPointD &pos, const TMouseEvent &) override {
    if (!(TVectorImageP)getImage(false)) return;
    m_leftButtonWasDown = true;
    if (m_type.getValue() == RECT) {
      SymmetryTool *symmetryTool = dynamic_cast<SymmetryTool *>(
          TTool::getTool("T_Symmetry", TTool::RasterImage));
      TPointD dpiScale       = getViewer()->getDpiScale();
      SymmetryObject symmObj = symmetryTool->getSymmetryObject();

      m_startRect = pos;

      if (symmetryTool && symmetryTool->isGuideEnabled()) {
        m_selectionRect = TRectD(m_startRect.x, m_startRect.y,
                                 m_startRect.x + 1, m_startRect.y + 1);

        // We'll use polyline
        m_polyline.reset();
        m_polyline.addSymmetryBrushes(symmObj.getLines(), symmObj.getRotation(),
                                      symmObj.getCenterPoint(),
                                      symmObj.isUsingLineSymmetry(), dpiScale);
        m_polyline.setRectangle(
            TPointD(m_selectionRect.x0, m_selectionRect.y0),
            TPointD(m_selectionRect.x1, m_selectionRect.y1));
      }
    } else if (m_type.getValue() == FREEHAND) {
        DEBUG_LOG("leftButtonDown(), FREEHAND\n");

        startFreehand(pos);
    } else if (m_strokeIndex1 != -1) {
      m_secondPoint = true;
    }
  }

  //-----------------------------------------------------------------------------

  void leftButtonDrag(const TPointD &pos, const TMouseEvent &) override {
    TVectorImageP vi(getImage(false));
    if (!vi) return;

    if (m_type.getValue() == RECT) {
      m_selectionRect = TRectD(
          std::min(m_startRect.x, pos.x), std::min(m_startRect.y, pos.y),
          std::max(m_startRect.x, pos.x), std::max(m_startRect.y, pos.y));

      if (m_polyline.size() > 1 && m_polyline.hasSymmetryBrushes()) {
        m_polyline.clear();
        m_polyline.setRectangle(
            TPointD(m_selectionRect.x0, m_selectionRect.y0),
            TPointD(m_selectionRect.x1, m_selectionRect.y1));
      }

      invalidate();
      return;
    }

    if (m_type.getValue() == FREEHAND) {
      //DEBUG_LOG("leftButtonDrag(), FREEHAND\n");

      // update the selection line while dragging
      freehandDrag(pos);

      return;
    }

    if (m_strokeIndex1 == -1 || !m_secondPoint) return;

    m_strokeIndex2 = -1;

    findSecondStroke(pos, vi, m_strokeIndex1, m_w1, m_strokeIndex2, m_w2);

    m_pos = pos;

    invalidate();
  }

  //-----------------------------------------------------------------------------

  void joinPointToPoint(const TVectorImageP &vi,
                        std::vector<TFilledRegionInf> *fillInfo) {
    DEBUG_LOG("joinPointToPoint()\n");
    int minindex = std::min(m_strokeIndex1, m_strokeIndex2);
    int maxindex = std::max(m_strokeIndex1, m_strokeIndex2);

    UndoAutoclose *autoCloseUndo = 0;
    TUndo *undo                  = 0;
    if (TTool::getApplication()->getCurrentObject()->isSpline())
      undo =
          new UndoPath(getXsheet()->getStageObject(getObjectId())->getSpline());
    else {
      TXshSimpleLevel *level =
          TTool::getApplication()->getCurrentLevel()->getSimpleLevel();
      std::vector<int> v(1);
      v[0]          = minindex;
      autoCloseUndo = new UndoAutoclose(level, getCurrentFid(), minindex,
                                        maxindex, fillInfo, v);
    }
    VIStroke *newStroke = vi->joinStroke(
        m_strokeIndex1, m_strokeIndex2,
        (m_w1 == 0.0)
            ? 0
            : vi->getStroke(m_strokeIndex1)->getControlPointCount() - 1,
        (m_w2 == 0.0)
            ? 0
            : vi->getStroke(m_strokeIndex2)->getControlPointCount() - 1,
        m_smooth.getValue());

    if (autoCloseUndo) {
      autoCloseUndo->m_newStroke   = cloneVIStroke(newStroke);
      autoCloseUndo->m_newStrokeId = vi->getStroke(minindex)->getId();
      undo                         = autoCloseUndo;
    }

    vi->notifyChangedStrokes(minindex);
    notifyImageChanged();
    TUndoManager::manager()->add(undo);
  }

  //-----------------------------------------------------------------------------

  void joinPointToLine(const TVectorImageP &vi,
                       std::vector<TFilledRegionInf> *fillInfo) {
    DEBUG_LOG("joinPointToLine()\n");
    TUndo *undo                  = 0;
    UndoAutoclose *autoCloseUndo = 0;
    if (TTool::getApplication()->getCurrentObject()->isSpline())
      undo =
          new UndoPath(getXsheet()->getStageObject(getObjectId())->getSpline());
    else {
      std::vector<int> v(2);
      v[0] = m_strokeIndex1;
      v[1] = m_strokeIndex2;
      TXshSimpleLevel *level =
          TTool::getApplication()->getCurrentLevel()->getSimpleLevel();
      autoCloseUndo = new UndoAutoclose(level, getCurrentFid(), m_strokeIndex1,
                                        -1, fillInfo, v);
    }

    VIStroke *newStroke;

    newStroke = vi->extendStroke(
        m_strokeIndex1, vi->getStroke(m_strokeIndex2)->getThickPoint(m_w2),
        (m_w1 == 0.0)
            ? 0
            : vi->getStroke(m_strokeIndex1)->getControlPointCount() - 1,
        m_smooth.getValue());

    if (autoCloseUndo) {
      autoCloseUndo->m_newStroke   = cloneVIStroke(newStroke);
      autoCloseUndo->m_newStrokeId = vi->getStroke(m_strokeIndex1)->getId();
      undo                         = autoCloseUndo;
    }

    vi->notifyChangedStrokes(m_strokeIndex1);
    notifyImageChanged();
    TUndoManager::manager()->add(undo);
  }

  //-----------------------------------------------------------------------------

  void joinLineToLine(const TVectorImageP &vi,
                      std::vector<TFilledRegionInf> *fillInfo) {
    DEBUG_LOG("joinLineToLine()\n");
    if (TTool::getApplication()->getCurrentObject()->isSpline())
      return;  // Caanot add vectros to spline... Spline can be only one
               // std::vector

    TThickPoint p1 = vi->getStroke(m_strokeIndex1)->getThickPoint(m_w1);
    TThickPoint p2 = vi->getStroke(m_strokeIndex2)->getThickPoint(m_w2);

    UndoAutoclose *autoCloseUndo = 0;
    std::vector<int> v(2);
    v[0] = m_strokeIndex1;
    v[1] = m_strokeIndex2;

    TXshSimpleLevel *level =
        TTool::getApplication()->getCurrentLevel()->getSimpleLevel();
    autoCloseUndo =
        new UndoAutoclose(level, getCurrentFid(), -1, -1, fillInfo, v);

    std::vector<TThickPoint> points(3);
    points[0]          = p1;
    points[1]          = 0.5 * (p1 + p2);
    points[2]          = p2;
    TStroke *auxStroke = new TStroke(points);
    auxStroke->setStyle(TTool::getApplication()->getCurrentLevelStyleIndex());
    auxStroke->outlineOptions() =
        vi->getStroke(m_strokeIndex1)->outlineOptions();

    int pos = vi->addStrokeToGroup(auxStroke, m_strokeIndex1);
    if (pos < 0) return;
    VIStroke *newStroke = vi->getVIStroke(pos);

    autoCloseUndo->m_newStrokePos = pos;
    autoCloseUndo->m_newStroke    = cloneVIStroke(newStroke);
    autoCloseUndo->m_newStrokeId  = vi->getStroke(pos)->getId();
    vi->notifyChangedStrokes(v, std::vector<TStroke *>());
    notifyImageChanged();
    TUndoManager::manager()->add(autoCloseUndo);
  }

  //-----------------------------------------------------------------------------

  void inline rearrangeClosingPoints(const TVectorImageP &vi,
                                     std::pair<int, double> &closingPoint,
                                     const TPointD &p) {
    DEBUG_LOG("rearrangeClosingPoints()\n");
    int erasedIndex = std::max(m_strokeIndex1, m_strokeIndex2);
    int joinedIndex = std::min(m_strokeIndex1, m_strokeIndex2);

    if (closingPoint.first == joinedIndex)
      closingPoint.second = vi->getStroke(joinedIndex)->getW(p);
    else if (closingPoint.first == erasedIndex) {
      closingPoint.first  = joinedIndex;
      closingPoint.second = vi->getStroke(joinedIndex)->getW(p);
    } else if (closingPoint.first > erasedIndex)
      closingPoint.first--;
  }

  //-------------------------------------------------------------------------------------

#define p2p 1
#define p2l 2
#define l2p 3
#define l2l 4

  inline TRectD interpolateRect(const TRectD &r1, const TRectD &r2, double t) {
    return TRectD(r1.x0 + (r2.x0 - r1.x0) * t, r1.y0 + (r2.y0 - r1.y0) * t,
                  r1.x1 + (r2.x1 - r1.x1) * t, r1.y1 + (r2.y1 - r1.y1) * t);
  }

  void tapeRect(const TVectorImageP &vi, const TRectD &rect,
                bool undoBlockStarted) {
    std::vector<TFilledRegionInf> *fillInformation =
        new std::vector<TFilledRegionInf>;
    ImageUtils::getFillingInformationOverlappingArea(vi, *fillInformation,
                                                     rect);

    bool initUndoBlock = false;

    std::vector<std::pair<int, double>> startPoints, endPoints;
    getClosingPoints(rect, m_autocloseFactor.getValue().first,
                     m_autocloseFactor.getValue().second, vi, startPoints,
                     endPoints);

    assert(startPoints.size() == endPoints.size());

    std::vector<TPointD> startP(startPoints.size()), endP(startPoints.size());

    if (!startPoints.empty()) {
      if (!undoBlockStarted) TUndoManager::manager()->beginBlock();
      for (UINT i = 0; i < startPoints.size(); i++) {
        startP[i] = vi->getStroke(startPoints[i].first)
                        ->getPoint(startPoints[i].second);
        endP[i] =
            vi->getStroke(endPoints[i].first)->getPoint(endPoints[i].second);
      }
    }

    for (UINT i = 0; i < startPoints.size(); i++) {
      m_strokeIndex1 = startPoints[i].first;
      m_strokeIndex2 = endPoints[i].first;
      m_w1           = startPoints[i].second;
      m_w2           = endPoints[i].second;
      int type       = doTape(vi, fillInformation, m_joinStrokes.getValue());
      if (type == p2p && m_strokeIndex1 != m_strokeIndex2) {
        for (UINT j = i + 1; j < startPoints.size(); j++) {
          rearrangeClosingPoints(vi, startPoints[j], startP[j]);
          rearrangeClosingPoints(vi, endPoints[j], endP[j]);
        }
      } else if (type == p2l ||
                 (type == p2p && m_strokeIndex1 == m_strokeIndex2)) {
        for (UINT j = i + 1; j < startPoints.size(); j++) {
          if (startPoints[j].first == m_strokeIndex1)
            startPoints[j].second =
                vi->getStroke(m_strokeIndex1)->getW(startP[j]);
          if (endPoints[j].first == m_strokeIndex1)
            endPoints[j].second = vi->getStroke(m_strokeIndex1)->getW(endP[j]);
        }
      }
    }
    if (!startPoints.empty() && !undoBlockStarted)
      TUndoManager::manager()->endBlock();
  }

  //----------------------------------------------------------------------
  
  void tapeFreehand(const TVectorImageP& vi, TStroke* stroke, bool undoBlockStarted) {
    DEBUG_LOG("tapeFreehand()\n");

    if (!vi || !stroke) {
      DEBUG_LOG("tapeFreehand(), (!vi || !stroke) so returning.\n");
      return;
    }

    std::vector<TFilledRegionInf>* fillInformation = new std::vector<TFilledRegionInf>;
    ImageUtils::getFillingInformationOverlappingArea(vi, *fillInformation, stroke->getBBox());

    std::vector<std::pair<int, double>> startPoints, endPoints;
    std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>> lineExtensions;
    getLineExtensionClosingPoints(stroke->getBBox(), vi, startPoints, endPoints, lineExtensions, false, false);

    assert(startPoints.size() == endPoints.size());

    std::vector<TPointD> startP(startPoints.size()), endP(startPoints.size());
    for (UINT i = 0; i < startPoints.size(); i++) {
      startP[i] = vi->getStroke(startPoints[i].first)->getPoint(startPoints[i].second);
      endP[i] = vi->getStroke(endPoints[i].first)->getPoint(endPoints[i].second);
    }

    if (!startPoints.empty()) {
      if (!undoBlockStarted) TUndoManager::manager()->beginBlock();

      for (UINT i = 0; i < startPoints.size(); i++) {
        startP[i] = vi->getStroke(startPoints[i].first)
          ->getPoint(startPoints[i].second);
        endP[i] =
          vi->getStroke(endPoints[i].first)->getPoint(endPoints[i].second);
      }

    }
    for (UINT i = 0; i < startPoints.size(); i++) {
      m_strokeIndex1 = startPoints[i].first;
      m_strokeIndex2 = endPoints[i].first;
      m_w1 = startPoints[i].second;
      m_w2 = endPoints[i].second;

      int type = doTape(vi, fillInformation, m_joinStrokes.getValue());

      if (type == p2p && m_strokeIndex1 != m_strokeIndex2) {
        for (UINT j = i + 1; j < startPoints.size(); j++) {
          rearrangeClosingPoints(vi, startPoints[j], startP[j]);
          rearrangeClosingPoints(vi, endPoints[j], endP[j]);
        }
      }
      else if (type == p2l || (type == p2p && m_strokeIndex1 == m_strokeIndex2)) {
        for (UINT j = i + 1; j < startPoints.size(); j++) {
          if (startPoints[j].first == m_strokeIndex1)
            startPoints[j].second = vi->getStroke(m_strokeIndex1)->getW(startP[j]);
          if (endPoints[j].first == m_strokeIndex1)
            endPoints[j].second = vi->getStroke(m_strokeIndex1)->getW(endP[j]);
        }
      }
    }

    if (!startPoints.empty() && !undoBlockStarted) TUndoManager::manager()->endBlock(); // ToDo - should this be endBlock if undoBlockStarted?

  }

  //----------------------------------------------------------------------

  void multiTapeFreehand(TStroke* stroke, TFrameId firstFrameId, TFrameId lastFrameId) {
    TTool::Application* app = TTool::getApplication();

    //TFrameId firstFrameId = m_firstFrameId;
    //TFrameId lastFrameId = getFrameId();

    DEBUG_LOG("multiTapeFreehand(stroke, firstFrameId, lastFrameId)\n");
    bool backward = false;
    if (firstFrameId > lastFrameId){
      std::swap(firstFrameId, lastFrameId);
      backward = true;
    }

    assert(firstFrameId <= lastFrameId);
    
    std::vector<TFrameId> allFids;
    m_level->getFids(allFids);

    std::vector<TFrameId>::iterator i0 = allFids.begin();
    while (i0 != allFids.end() && *i0 < firstFrameId) i0++;
    std::vector<TFrameId>::iterator i1 = i0;
    while (i1 != allFids.end() && *i1 <= lastFrameId) i1++;
    
    std::vector<TFrameId> fids(i0, i1);
    assert(io < i1);
    
    int m = fids.size();
    assert(m > 0);

    if (fids.empty()) return;

    enum TInbetween::TweenAlgorithm algorithm = TInbetween::LinearInterpolation;
    switch (m_multi.getIndex()) {
    case 2: algorithm = TInbetween::EaseInInterpolation; break;
    case 3: algorithm = TInbetween::EaseOutInterpolation; break;
    case 4: algorithm = TInbetween::EaseInOutInterpolation; break;
    }

    TUndoManager::manager()->beginBlock();
    for (int i = 0; i < (int)fids.size(); ++i) {
      double t = fids.size() > 1 ? (double)i / (double)(fids.size() - 1) : 0.5;
      t = TInbetween::interpolation(t, algorithm);

      TFrameId fid = fids[i];
      app->getCurrentFrame()->setFid(fid);
      TVectorImageP vi = m_level->getFrame(fid, true);
      if (!vi) continue;

      tapeFreehand(vi, stroke, true);  // Apply tape to each frame
    }
    TUndoManager::manager()->endBlock();

    TTool::getApplication()->getCurrentXsheet()->notifyXsheetChanged();
  }

  //----------------------------------------------------------------------

  void multiTapeFreehand(TStroke* stroke, int firstFrameIdx, int lastFrameIdx) {
    TTool::Application* app = TTool::getApplication();
    DEBUG_LOG("multiTapeFreehand(stroke, firstFrameIdx, lastFrameIdx)\n");
    TFrameId firstFrameId = m_firstFrameId;
    TFrameId lastFrameId = getFrameId();

    if (firstFrameId > lastFrameId)
      std::swap(firstFrameId, lastFrameId);

    std::vector<TFrameId> allFids;
    m_level->getFids(allFids);

    std::vector<TFrameId>::iterator i0 = allFids.begin();
    while (i0 != allFids.end() && *i0 < firstFrameId) i0++;
    std::vector<TFrameId>::iterator i1 = i0;
    while (i1 != allFids.end() && *i1 <= lastFrameId) i1++;
    std::vector<TFrameId> fids(i0, i1);

    if (fids.empty()) return;

    enum TInbetween::TweenAlgorithm algorithm = TInbetween::LinearInterpolation;
    switch (m_multi.getIndex()) {
    case 2: algorithm = TInbetween::EaseInInterpolation; break;
    case 3: algorithm = TInbetween::EaseOutInterpolation; break;
    case 4: algorithm = TInbetween::EaseInOutInterpolation; break;
    }

    TUndoManager::manager()->beginBlock();
    for (int i = 0; i < (int)fids.size(); ++i) {
      double t = fids.size() > 1 ? (double)i / (double)(fids.size() - 1) : 0.5;
      t = TInbetween::interpolation(t, algorithm);

      TFrameId fid = fids[i];
      app->getCurrentFrame()->setFid(fid);
      TVectorImageP vi = m_level->getFrame(fid, true);
      if (!vi) continue;

      tapeFreehand(vi, stroke, true);  // Apply tape to each frame
    }
    TUndoManager::manager()->endBlock();

    TTool::getApplication()->getCurrentXsheet()->notifyXsheetChanged();
  }

    //----------------------------------------------------------------------
    
  void multiTapeRect(TFrameId firstFrameId, TFrameId lastFrameId) {
    TTool::Application *app = TTool::getApplication();

    bool backward = false;
    if (firstFrameId > lastFrameId) {
      std::swap(firstFrameId, lastFrameId);
      backward = true;
    }
    assert(firstFrameId <= lastFrameId);
    std::vector<TFrameId> allFids;
    m_level->getFids(allFids);

    std::vector<TFrameId>::iterator i0 = allFids.begin();
    while (i0 != allFids.end() && *i0 < firstFrameId) i0++;
    if (i0 == allFids.end()) return;
    std::vector<TFrameId>::iterator i1 = i0;
    while (i1 != allFids.end() && *i1 <= lastFrameId) i1++;
    assert(i0 < i1);
    std::vector<TFrameId> fids(i0, i1);
    int m = fids.size();
    assert(m > 0);

    enum TInbetween::TweenAlgorithm algorithm = TInbetween::LinearInterpolation;
    if (m_multi.getIndex() == 2) {  // EASE_IN_INTERPOLATION)
      algorithm = TInbetween::EaseInInterpolation;
    } else if (m_multi.getIndex() == 3) {  // EASE_OUT_INTERPOLATION)
      algorithm = TInbetween::EaseOutInterpolation;
    } else if (m_multi.getIndex() == 4) {  // EASE_IN_OUT_INTERPOLATION)
      algorithm = TInbetween::EaseInOutInterpolation;
    }

    TUndoManager::manager()->beginBlock();
    //for (int i = 0; i <= m; ++i) {
    for (int i = 0; i < m; ++i) {
      TFrameId fid     = fids[i];
      TVectorImageP vi = (TVectorImageP)m_level->getFrame(fid, true);
      if (!vi) continue;
      double t = m > 1 ? (double)i / (double)(m - 1) : 0.5;
      t        = TInbetween::interpolation(t, algorithm);
      app->getCurrentFrame()->setFid(fid);

      tapeRect(vi, interpolateRect(m_firstRect, m_selectionRect, t), true);

      if (m_firstPolyline.hasSymmetryBrushes()) {
        for (int i = 1; i < m_firstPolyline.getBrushCount(); i++) {
          TStroke *firstSymmStroke = m_firstPolyline.makeRectangleStroke(i);
          TRectD firstSymmSelectionRect = firstSymmStroke->getBBox();
          TStroke *symmStroke           = m_polyline.makeRectangleStroke(i);
          TRectD symmSelectionRect      = symmStroke->getBBox();
          tapeRect(
              vi, interpolateRect(firstSymmSelectionRect, symmSelectionRect, t),
              true);
        }
      }
    }
    TUndoManager::manager()->endBlock();

    TTool::getApplication()->getCurrentXsheet()->notifyXsheetChanged();
  }

  //----------------------------------------------------------------------

  void multiTapeRect(int firstFrameIdx, int lastFrameIdx) {
    bool backward = false;
    if (firstFrameIdx > lastFrameIdx) {
      std::swap(firstFrameIdx, lastFrameIdx);
      backward = true;
    }
    assert(firstFrameIdx <= lastFrameIdx);

    TTool::Application *app = TTool::getApplication();
    TFrameId lastFrameId;
    int col = app->getCurrentColumn()->getColumnIndex();
    int row;

    std::vector<std::pair<int, TXshCell>> cellList;

    for (row = firstFrameIdx; row <= lastFrameIdx; row++) {
      TXshCell cell = app->getCurrentXsheet()->getXsheet()->getCell(row, col);
      if (cell.isEmpty()) continue;
      TFrameId fid = cell.getFrameId();
      if (lastFrameId == fid) continue;  // Skip held cells
      cellList.push_back(std::pair<int, TXshCell>(row, cell));
      lastFrameId = fid;
    }

    int m = cellList.size();

    enum TInbetween::TweenAlgorithm algorithm = TInbetween::LinearInterpolation;
    if (m_multi.getIndex() == 2) {  // EASE_IN_INTERPOLATION)
      algorithm = TInbetween::EaseInInterpolation;
    } else if (m_multi.getIndex() == 3) {  // EASE_OUT_INTERPOLATION)
      algorithm = TInbetween::EaseOutInterpolation;
    } else if (m_multi.getIndex() == 4) {  // EASE_IN_OUT_INTERPOLATION)
      algorithm = TInbetween::EaseInOutInterpolation;
    }

    TUndoManager::manager()->beginBlock();
    for (int i = 0; i < m; ++i) {
      row              = cellList[i].first;
      TXshCell cell    = cellList[i].second;
      TFrameId fid     = cell.getFrameId();
      TVectorImageP vi = (TVectorImageP)cell.getImage(true);
      if (!vi) continue;
      double t = m > 1 ? (double)i / (double)(m - 1) : 0.5;
      t        = TInbetween::interpolation(t, algorithm);
      app->getCurrentFrame()->setFrame(row);

      tapeRect(vi, interpolateRect(m_firstRect, m_selectionRect, t), true);

      if (m_firstPolyline.hasSymmetryBrushes()) {
        for (int j = 1; j < m_firstPolyline.getBrushCount(); j++) {
          TStroke *firstSymmStroke = m_firstPolyline.makeRectangleStroke(j);
          TRectD firstSymmSelectionRect = firstSymmStroke->getBBox();
          TStroke *symmStroke           = m_polyline.makeRectangleStroke(j);
          TRectD symmSelectionRect      = symmStroke->getBBox();
          tapeRect(
              vi, interpolateRect(firstSymmSelectionRect, symmSelectionRect, t),
              true);
        }
      }
    }
    TUndoManager::manager()->endBlock();

    TTool::getApplication()->getCurrentXsheet()->notifyXsheetChanged();
  }

  int doTape(const TVectorImageP &vi,
             std::vector<TFilledRegionInf> *fillInformation, bool joinStrokes) {
    DEBUG_LOG("doTape()\n");
    int type;
    if (!joinStrokes)
      type = l2l;
    else {
      type = (m_w1 == 0.0 || m_w1 == 1.0)
                 ? ((m_w2 == 0.0 || m_w2 == 1.0) ? p2p : p2l)
                 : ((m_w2 == 0.0 || m_w2 == 1.0) ? l2p : l2l);
      if (type == l2p) {
        std::swap(m_strokeIndex1, m_strokeIndex2);
        std::swap(m_w1, m_w2);
        type = p2l;
      }
    }

    switch (type) {
    case p2p:
      joinPointToPoint(vi, fillInformation);
      break;
    case p2l:
      joinPointToLine(vi, fillInformation);
      break;
    case l2l:
      joinLineToLine(vi, fillInformation);
      break;
    default:
      assert(false);
      break;
    }
    return type;
  }

  void tapeStrokes(TVectorImageP vi, int strokeIndex1, int strokeIndex2) {
    DEBUG_LOG("tapeStrokes()\n");
    m_strokeIndex1 = strokeIndex1;
    m_strokeIndex2 = strokeIndex2;

    QMutexLocker lock(vi->getMutex());
    std::vector<TFilledRegionInf> *fillInformation =
        new std::vector<TFilledRegionInf>;
    ImageUtils::getFillingInformationOverlappingArea(
        vi, *fillInformation, vi->getStroke(strokeIndex1)->getBBox() +
                                  vi->getStroke(strokeIndex2)->getBBox());

    doTape(vi, fillInformation, m_joinStrokes.getValue());
  }

  //-------------------------------------------------------------------------------

  void leftButtonUp(const TPointD &pos, const TMouseEvent &e) override {
    
    if (!m_leftButtonWasDown) {
      return;
    }else
    {
      m_leftButtonWasDown = false;
    }

    TTool::Application *app = TTool::getApplication();

    TVectorImageP vi(getImage(true));

    DEBUG_LOG("leftButtonUp(), vi exists:" << vi << ", m_type.getValue():" << m_type.getValueAsString() << ", app->getCurrentObject()->objectName():" << app->getCurrentObject()->objectName().toStdString() << "\n");

    if (vi && m_type.getValue() == RECT) {
      bool isEditingLevel = app->getCurrentFrame()->isEditingLevel();

      if (m_multi.getIndex()) {
        if (!m_firstFrameSelected) {
          m_currCell     = std::pair<int, int>(getColumnIndex(), getFrame());
          m_firstRect    = m_selectionRect;
          m_firstFrameId = m_veryFirstFrameId = getFrameId();
          m_firstFrameIdx                     = getFrame();
          m_firstPolyline                     = m_polyline;
          m_level = app->getCurrentLevel()->getLevel()
                        ? app->getCurrentLevel()->getSimpleLevel()
                        : 0;
          m_firstFrameSelected = true;
        } else {
          if (app->getCurrentFrame()->isEditingScene())
            multiTapeRect(m_firstFrameIdx, getFrame());
          else
            multiTapeRect(m_firstFrameId, getFrameId());

          invalidate(m_selectionRect.enlarge(2));
          if (e.isShiftPressed()) {
            m_currCell     = std::pair<int, int>(getColumnIndex(), getFrame());
            m_firstRect    = m_selectionRect;
            m_firstFrameId = m_veryFirstFrameId = getFrameId();
            m_firstFrameIdx                     = getFrame();
            m_firstPolyline                     = m_polyline;
          } else {
            if (app->getCurrentFrame()->isEditingScene()) {
              app->getCurrentColumn()->setColumnIndex(m_currCell.first);
              app->getCurrentFrame()->setFrame(m_currCell.second);
            } else {
              app->getCurrentFrame()->setFid(m_veryFirstFrameId);
            }
            m_firstFrameSelected = false;
          }

          m_selectionRect = TRectD();
          m_startRect     = TPointD();
          m_polyline.reset();
        }
        return;
      }

      if (m_polyline.hasSymmetryBrushes())
        TUndoManager::manager()->beginBlock();

      tapeRect(vi, m_selectionRect, m_polyline.hasSymmetryBrushes());

      if (m_polyline.hasSymmetryBrushes()) {
        for (int i = 1; i < m_polyline.getBrushCount(); i++) {
          TStroke *symmStroke      = m_polyline.makeRectangleStroke(i);
          TRectD symmSelectionRect = symmStroke->getBBox();
          tapeRect(vi, symmSelectionRect, true);
        }

        TUndoManager::manager()->endBlock();
      }

      m_selectionRect = TRectD();
      m_startRect     = TPointD();
      m_polyline.reset();
      notifyImageChanged();
      invalidate();
      return;
    }

    if (vi && m_type.getValue() == FREEHAND) {
      DEBUG_LOG("leftButtonUp() FREEHAND\n");
      bool isEditingLevel = app->getCurrentFrame()->isEditingLevel();
      bool isEditingScene = app->getCurrentFrame()->isEditingScene();
      DEBUG_LOG("app->getCurrentFrame()->isEditingLevel():" << isEditingLevel << ", app->getCurrentFrame()->isEditingScene():" << app->getCurrentFrame()->isEditingScene());
      closeFreehand(pos);  // complete the freehand stroke

      if (m_multi.getIndex()) {
        DEBUG_LOG("leftButtonUp() FREEHAND, Multi, first click\n");
        // first click: store info
        if (!m_firstFrameSelected) {
          DEBUG_LOG("leftButtonUp() FREEHAND, Multi, first frame select\n");
          m_currCell = std::pair<int, int>(getColumnIndex(), getFrame());
          m_firstFreehandLasso = m_stroke;
          m_firstFrameId = m_veryFirstFrameId = getFrameId();
          m_firstFrameIdx                     = getFrame();
          m_firstPolyline                     = m_polyline;
          m_level = app->getCurrentLevel()->getLevel()
            ? app->getCurrentLevel()->getSimpleLevel()
            : 0;
          m_firstFrameSelected = true;
        } else {
          DEBUG_LOG("leftButtonUp() FREEHAND, Multi, additional frames select\n");
          if (app->getCurrentFrame()->isEditingScene()){
            multiTapeFreehand(m_stroke, m_firstFrameIdx, getFrame()); // int values, level strip
          } else{
            multiTapeFreehand(m_stroke, m_firstFrameId, getFrameId()); // TFrame values, xsheet/timeline
          }

          //invalidate(m_selectionRect.enlarge(2)); // is this to clear the selection border on screen? if so, need bbox as rect for lasso probably.
          invalidate(m_stroke->getBBox().enlarge(2));

          if (e.isShiftPressed()) {
            m_currCell = std::pair<int, int>(getColumnIndex(), getFrame());
            m_firstFreehandLasso = m_stroke; //m_firstRect = m_selectionRect;
            m_firstFrameId = m_veryFirstFrameId = getFrameId();
            m_firstFrameIdx                     = getFrame();
            m_firstPolyline                     = m_polyline;
          } else {
            //if (app->getCurrentFrame()->isEditingScene()) { // xsheet/timeline
            if (!isEditingLevel) { // xsheet/timeline
              DEBUG_LOG("<<< leave me active on the xsheet/timeline >>>\n");             
              app->getCurrentColumn()->setColumnIndex(m_currCell.first);
              app->getCurrentFrame()->setFrame(m_currCell.second);
            } else{ // level strip
              DEBUG_LOG("<<< leave me active on the Level Strip >>>\n");
              app->getCurrentFrame()->setFid(m_veryFirstFrameId);
            }
            m_firstFrameSelected = false;
          }

          //m_selectionRect = TRectD();
          //m_startRect = TPointD();
          //m_polyline.reset();
          m_stroke = new TStroke();
          m_track.reset();
          //m_firstFreehandLasso = m_stroke;
          m_polyline.reset();
        }
        return;
      }
      DEBUG_LOG("leftButtonUp() FREEHAND, m_track.hasSymmetryBrushes():" << m_track.hasSymmetryBrushes() << ", brush count:" << m_track.getBrushCount() << "\n");

      if (m_polyline.hasSymmetryBrushes()) {
        DEBUG_LOG("leftButtonUp() FREEHAND, has symmetry brushes, begin undo block\n");
        TUndoManager::manager()->beginBlock();
      }
      //TRectD strokeBBox;
      //if (m_stroke) {
      //  strokeBBox = m_stroke->getBBox();
      //  DEBUG_LOG("leftButtonUp() FREEHAND, m_stroke, x0,y0: " << strokeBBox.x0 << "," << strokeBBox.y0 << " x1,y1: " << strokeBBox.x1 << "," << strokeBBox.y1 << "\n");
      //}

      tapeFreehand(vi, m_stroke, m_track.hasSymmetryBrushes());

      if (m_track.hasSymmetryBrushes()) {
        DEBUG_LOG("leftButtonUp() FREEHAND, has symmetry brushes, iterate over each\n");
        for (int i = 1; i < m_track.getBrushCount(); i++) {
          DEBUG_LOG("leftButtonUp() FREEHAND, has symmetry brushes, int i:" << i << "\n");
          double error = (30.0 / 11) * sqrt(getPixelSize() * getPixelSize());
          TStroke* symmStroke = m_track.makeStroke(error, i); // make freehand stroke
          //TRectD strokeBBox;
          //strokeBBox = symmStroke->getBBox();
          //DEBUG_LOG("leftButtonUp() FREEHAND, symmStroke, x0,y0: " << strokeBBox.x0 << "," << strokeBBox.y0 << " x1,y1: " << strokeBBox.x1 << "," << strokeBBox.y1 << "\n");
          tapeFreehand(vi, symmStroke, true);
        }

        TUndoManager::manager()->endBlock();
      }

      //m_selectionRect = TRectD();
      //m_startRect = TPointD();
      // 
      //m_polyline.reset();
      //notifyImageChanged();
      //invalidate();

      m_track.reset();
      notifyImageChanged();
      invalidate();
      return;
    }

    if (!vi || m_strokeIndex1 == -1 || !m_secondPoint || m_strokeIndex2 == -1) {
      m_strokeIndex1 = -1;
      m_strokeIndex2 = -1;
      m_w1           = -1.0;
      m_w2           = -1.0;
      m_secondPoint  = false;
      return;
    }

    m_secondPoint = false;

    SymmetryTool *symmetryTool = dynamic_cast<SymmetryTool *>(
        TTool::getTool("T_Symmetry", TTool::RasterImage));
    if (symmetryTool && symmetryTool->isGuideEnabled())
      TUndoManager::manager()->beginBlock();

    TStroke *stroke1   = vi->getStroke(m_strokeIndex1);
    TStroke *stroke2   = vi->getStroke(m_strokeIndex2);
    TThickPoint point1 = stroke1->getPoint(m_w1);
    TThickPoint point2 = stroke2->getPoint(m_w2);

    tapeStrokes(vi, m_strokeIndex1, m_strokeIndex2);

    if (symmetryTool && symmetryTool->isGuideEnabled()) {
      std::vector<TPointD> firstPts = symmetryTool->getSymmetryPoints(
          point1, TPointD(), getViewer()->getDpiScale());
      std::vector<TPointD> secondPts = symmetryTool->getSymmetryPoints(
          point2, TPointD(), getViewer()->getDpiScale());

      for (int i = 1; i < firstPts.size(); i++) {
        int strokeIndex1 = vi->getStrokeIndexAtPos(firstPts[i], getPixelSize());
        if (strokeIndex1 == -1) continue;
        int strokeIndex2 =
            vi->getStrokeIndexAtPos(secondPts[i], getPixelSize());
        if (strokeIndex2 == -1) continue;
        tapeStrokes(vi, strokeIndex1, strokeIndex2);
      }

      TUndoManager::manager()->endBlock();
    }

    invalidate();

    m_strokeIndex2 = -1;
    m_w1           = -1.0;
    m_w2           = -1.0;
  }

  //-----------------------------------------------------------------------------

  void onEnter() override {
    //      getApplication()->editImage();
    //    m_selectionRect = TRectD();
    m_startRect     = TPointD();
    std::cout << "VectorTapeTool onEnter()\n";
    //setGapCheck();
  }

  void onActivate() override {

    if (!m_firstTime) {
      std::cout << "VectorTapeTool onActivate(), not my first time.\n";
      setGapCheck();
      return;
    }
    std::cout << "VectorTapeTool onActivate(), first time.\n";

    std::wstring s = ::to_wstring(TapeMode.getValue());
    if (s != L"") m_mode.setValue(s);
    s = ::to_wstring(TapeType.getValue());
    if (s != L"") m_type.setValue(s);
    m_autocloseFactor.setValue(
        TDoublePairProperty::Value(AutocloseFactorMin, AutocloseFactor));
    m_smooth.setValue(TapeSmooth ? 1 : 0);
    m_joinStrokes.setValue(TapeJoinStrokes ? 1 : 0);
    m_multi.setIndex(TapeRange);
    m_firstTime     = false;
    m_selectionRect = TRectD();
    m_startRect     = TPointD();
    m_dehookFactor.setValue(TDoublePairProperty::Value(TapeDehookFactorMin, TapeDehookFactorMax));
    m_dehookAngleThreshold.setValue(TapeDehookAngleThreshold);
    m_lineExtensionAngle.setValue(LineExtensionAngle);
    //std::cout << "VectorTapeTool onActivate() setValue on TapeStartAt to:" << TapeStartAt << "\n";
    //m_startAt.setValue(TapeStartAt);
    //std::cout << "VectorTapeTool onActivate() setValue on TapeIncBy to:" << TapeIncBy << "\n";
    //m_incBy.setValue(TapeIncBy);
    
    std::cout << "VectorTapeTool onActivate() call setGapCheck().\n";
    setGapCheck();
    std::cout << "VectorTapeTool onActivate() completed.\n";
  }

  int getCursorId() const override {
    int ret = ToolCursor::TapeCursor;
    if (m_type.getValue() == RECT) ret = ret | ToolCursor::Ex_Rectangle;
    if (ToonzCheck::instance()->getChecks() & ToonzCheck::eBlackBg)
      ret = ret | ToolCursor::Ex_Negate;
    return ret;
  }

} vectorTapeTool;

// TTool *getAutocloseTool() {return &autocloseTool;}

#pragma once

#ifndef XSHEETVIEWER_H
#define XSHEETVIEWER_H

#include <QFrame>
#include <QScrollArea>
#include <QKeyEvent>
#include "xshcellviewer.h"
#include "xshcolumnviewer.h"
#include "xshrowviewer.h"
#include "xshnoteviewer.h"
#include "quicktoolbar.h"
#include "layerfooterpanel.h"
#include "cellkeyframeselection.h"
#include "saveloadqsettings.h"
#include "toonzqt/spreadsheetviewer.h"
#include "orientation.h"
#include "xshbreadcrumbs.h"
#include <boost/optional.hpp>

using boost::optional;

#define XSHEET_FONT_PX_SIZE 12
#define H_ADJUST 2

// forward declaration
class TXsheet;
class TCellSelection;
class TKeyframeSelection;
class TColumnSelection;
class TSelection;
class TXshCell;
class TStageObjectId;

enum TXshButtonType {
  PREVIEW_ON_XSHBUTTON = 1,
  PREVIEW_OFF_XSHBUTTON,
  CAMSTAND_ON_XSHBUTTON,
  CAMSTAND_OFF_XSHBUTTON,
  CAMSTAND_TRANSP_XSHBUTTON,
  LOCK_ON_XSHBUTTON,
  LOCK_OFF_XSHBUTTON,
  CONFIG_XSHBUTTON,
  UNIFIED_TRANSP_XSHBUTTON
};

namespace XsheetGUI {

//=============================================================================
// Constant definition
//-----------------------------------------------------------------------------

extern const int ColumnWidth;
extern const int RowHeight;

const int NoteWidth  = 70;
const int NoteHeight = 18;

// TZP column
const QColor LevelColumnColor(127, 219, 127);
const QColor LevelColumnBorderColor(47, 82, 47);
const QColor SelectedLevelColumnColor(191, 237, 191);

// PLI column
const QColor VectorColumnColor(212, 212, 133);
const QColor VectorColumnBorderColor(79, 79, 49);
const QColor SelectedVectorColumnColor(234, 234, 194);

// SubXsheet column
const QColor ChildColumnColor(214, 154, 219);
const QColor ChildColumnBorderColor(80, 57, 82);
const QColor SelectedChildColumnColor(235, 205, 237);

// Raster image column
const QColor FullcolorColumnColor(154, 214, 219);
const QColor FullcolorColumnBorderColor(57, 80, 82);
const QColor SelectedFullcolorColumnColor(205, 235, 237);

// Palette column
const QColor PaletteColumnColor(42, 171, 154);
const QColor PaletteColumnBorderColor(15, 62, 56);
const QColor SelectedPaletteColumnColor(146, 221, 202);

// Fx column
const QColor FxColumnColor(130, 129, 93);
const QColor FxColumnBorderColor(48, 48, 35);
const QColor SelectedFxColumnColor(193, 192, 174);

// Reference column
const QColor ReferenceColumnColor(171, 171, 171);
const QColor ReferenceColumnBorderColor(62, 62, 62);
const QColor SelectedReferenceColumnColor(213, 213, 213);

// Sound column
const QColor SoundColumnColor(175, 185, 115);
const QColor SoundColumnBorderColor(110, 130, 90);
const QColor SelectedSoundColumnColor(215, 215, 180);

const QColor SoundColumnHlColor(245, 255, 230);
const QColor SoundColumnTrackColor(90, 100, 45);

const QColor SoundColumnExtenderColor(235, 255, 115);

const QColor EmptySoundColumnColor(240, 255, 240);

const QColor ColorSelection(190, 210, 240, 170);

const QColor SoundTextColumnColor(200, 200, 200);
const QColor SoundTextColumnBorderColor(140, 140, 140);

const QColor MeshColumnColor(200, 130, 255);
const QColor MeshColumnBorderColor(105, 70, 135);
const QColor SelectedMeshColumnColor(216, 180, 245);

// Empty column
const QColor EmptyColumnColor(124, 124, 124);
// Occupied column
const QColor NotEmptyColumnColor(164, 164, 164);

const QColor SelectedEmptyCellColor(210, 210, 210);
const QColor SmartTabColor(255, 255, 255, 150);

const QColor XsheetBGColor(212, 208, 200);
// Xsheet horizontal lines
const QColor NormalHLineColor(146, 144, 146);
const QColor IntervalHLineColor(0, 255, 246);

// column header
const QColor EmptyColumnHeadColor(200, 200, 200);
const QColor MaskColumnHeadColor(233, 118, 116);
// const QColor PreviewVisibleColor(200, 200, 100);
// const QColor CamStandVisibleColor(235, 144, 107);

const QColor XsheetColumnNameBgColor(255, 255, 255, 90);
const QColor XsheetDragBarHighlightColor(255, 255, 187, 255);

const QColor XsheetPreviewButtonBgOnColor(200, 200, 100, 255);
const QColor XsheetPreviewButtonBgOffColor(255, 255, 255, 0);

const QColor XsheetCamstandButtonBgOnColor(235, 144, 107, 255);
const QColor XsheetCamstandButtonBgOffColor(255, 255, 255, 0);

const QColor XsheetLockButtonBgOnColor(255, 255, 255, 0);
const QColor XsheetLockButtonBgOffColor(255, 255, 255, 0);

const QColor XsheetConfigButtonBgColor(255, 255, 255, 0);

const QColor TimelineButtonBgOnColor(255, 255, 255, 0);
const QColor TimelineButtonBgOffColor(255, 255, 255, 0);

const QColor TimelineConfigButtonBgColor(255, 255, 255, 0);

// RowArea
const QColor RowAreaBGColor(164, 164, 164);
const QColor CurrentFrameBGColor(210, 210, 210);

}  // namespace XsheetGUI

//=============================================================================
// XsheetScrollArea
//-----------------------------------------------------------------------------

class XsheetScrollArea final : public QScrollArea {
  Q_OBJECT

public:
  XsheetScrollArea(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags())
      : QScrollArea(parent) {
    setObjectName("xsheetScrollArea");
    setFrameStyle(QFrame::StyledPanel);
  }
  ~XsheetScrollArea() {}

protected:
  void keyPressEvent(QKeyEvent *event) override { event->ignore(); }
  void wheelEvent(QWheelEvent *event) override { event->ignore(); }
};

//=============================================================================
// XsheetViewer
//-----------------------------------------------------------------------------

//! Note: some refactoring is needed. XsheetViewer is going to derive from
//! SpreadsheetViewer.

class XsheetViewer final : public QFrame, public SaveLoadQSettings {
  Q_OBJECT

  QColor m_lightLightBgColor;
  QColor m_lightBgColor;
  QColor m_bgColor;  // row area background
  QColor m_darkBgColor;
  QColor m_lightLineColor;  // horizontal lines (146,144,146)
  QColor m_darkLineColor;
  QColor m_columnIconLineColor;
  QColor m_timelineIconLineColor;

  Q_PROPERTY(QColor LightLightBGColor READ getLightLightBGColor WRITE
                 setLightLightBGColor)
  Q_PROPERTY(QColor LightBGColor READ getLightBGColor WRITE setLightBGColor)
  Q_PROPERTY(QColor BGColor READ getBGColor WRITE setBGColor)
  Q_PROPERTY(QColor DarkBGColor READ getDarkBGColor WRITE setDarkBGColor)
  Q_PROPERTY(
      QColor LightLineColor READ getLightLineColor WRITE setLightLineColor)
  Q_PROPERTY(QColor DarkLineColor READ getDarkLineColor WRITE setDarkLineColor)
  Q_PROPERTY(QColor ColumnIconLineColor READ getColumnIconLineColor WRITE
                 setColumnIconLineColor)
  Q_PROPERTY(QColor TimelineIconLineColor READ getTimelineIconLineColor WRITE
                 setTimelineIconLineColor)

  // Row
  QColor m_currentRowBgColor;        // current frame / column (210,210,210)
  QColor m_markerLineColor;          // marker lines (0, 255, 246)
  QColor m_secMarkerLineColor;       // second marker lines
  QColor m_selectedMarkerLineColor;  // marker lines in selected cells
  QColor m_verticalLineColor;        // vertical lines
  QColor m_verticalLineHeadColor;    // vertical lines in column head
  QColor m_textColor;                // text color (black)
  QColor m_errorTextColor;           // error text color (red, probably)
  QColor m_selectedTextColor;        // text color for the selected cells
  QColor m_frameTextColor;           // text color for frame numbers
  QColor m_keyframeLineColor;        // color of keyframe lines
  QColor m_currentFrameTextColor;    // text color for the current frame row
  QColor m_previewFrameTextColor;    // frame number in preview range (blue)
  QColor m_onionSkinAreaBgColor;
  QColor m_frameRangeMarkerLineColor;  // timeline frame markers
  Q_PROPERTY(QColor CurrentRowBgColor READ getCurrentRowBgColor WRITE
                 setCurrentRowBgColor)
  Q_PROPERTY(
      QColor MarkerLineColor READ getMarkerLineColor WRITE setMarkerLineColor)
  Q_PROPERTY(QColor SecMarkerLineColor READ getSecMarkerLineColor WRITE
                 setSecMarkerLineColor)
  Q_PROPERTY(QColor SelectedMarkerLineColor READ getSelectedMarkerLineColor
                 WRITE setSelectedMarkerLineColor)
  Q_PROPERTY(QColor VerticalLineColor READ getVerticalLineColor WRITE
                 setVerticalLineColor)
  Q_PROPERTY(QColor VerticalLineHeadColor READ getVerticalLineHeadColor WRITE
                 setVerticalLineHeadColor)
  Q_PROPERTY(QColor TextColor READ getTextColor WRITE setTextColor)
  Q_PROPERTY(
      QColor ErrorTextColor READ getErrorTextColor WRITE setErrorTextColor)
  Q_PROPERTY(QColor CurrentFrameTextColor READ getCurrentFrameTextColor WRITE
                 setCurrentFrameTextColor)
  Q_PROPERTY(QColor SelectedTextColor READ getSelectedTextColor WRITE
                 setSelectedTextColor)
  Q_PROPERTY(QColor FrameTextColor READ getFrameTextColor WRITE
                 setFrameTextColor)
  Q_PROPERTY(QColor KeyframeLineColor READ getKeyframeLineColor WRITE
                 setKeyframeLineColor)
  Q_PROPERTY(QColor PreviewFrameTextColor READ getPreviewFrameTextColor WRITE
                 setPreviewFrameTextColor)
  Q_PROPERTY(QColor OnionSkinAreaBgColor READ getOnionSkinAreaBgColor WRITE
                 setOnionSkinAreaBgColor)
  Q_PROPERTY(QColor FrameRangeMarkerLineColor READ getFrameRangeMarkerLineColor
                 WRITE setFrameRangeMarkerLineColor)
  // Column
  QColor m_columnTextColor;
  Q_PROPERTY(QColor ColumnTextColor READ getColumnTextColor WRITE
                 setColumnTextColor)
  QColor m_highlightColumnTextColor;
  Q_PROPERTY(QColor HighlightColumnTextColor READ
                 getHighlightColumnTextColor WRITE
                 setHighlightColumnTextColor)
  QColor m_emptyColumnHeadColor;  // empty column header (200,200,200)
  Q_PROPERTY(QColor EmptyColumnHeadColor READ getEmptyColumnHeadColor WRITE
                 setEmptyColumnHeadColor)

  // Cell
  QColor m_emptyCellColor;          // empty cell (124,124,124)
  QColor m_notEmptyColumnColor;     // occupied column (164,164,164)
  QColor m_selectedEmptyCellColor;  // selected empty cell (210,210,210)
  QColor m_levelEndColor;           // end level cross
  Q_PROPERTY(
      QColor EmptyCellColor READ getEmptyCellColor WRITE setEmptyCellColor)
  Q_PROPERTY(QColor NotEmptyColumnColor READ getNotEmptyColumnColor WRITE
                 setNotEmptyColumnColor)
  Q_PROPERTY(QColor SelectedEmptyCellColor READ getSelectedEmptyCellColor WRITE
                 setSelectedEmptyCellColor)
  Q_PROPERTY(QColor LevelEndColor READ getLevelEndColor WRITE setLevelEndColor)

  // Cell focus
  //  QColor m_cellFocusColor;
  //  Q_PROPERTY(
  //      QColor CellFocusColor READ getCellFocusColor WRITE setCellFocusColor)

  // Play range
  QColor m_playRangeColor;
  Q_PROPERTY(
      QColor PlayRangeColor READ getPlayRangeColor WRITE setPlayRangeColor)

  // TZP column
  QColor m_levelColumnColor;          //(127,219,127)
  QColor m_levelColumnBorderColor;    //(47,82,47)
  QColor m_selectedLevelColumnColor;  //(191,237,191)
  Q_PROPERTY(QColor LevelColumnColor READ getLevelColumnColor WRITE
                 setLevelColumnColor)
  Q_PROPERTY(QColor LevelColumnBorderColor READ getLevelColumnBorderColor WRITE
                 setLevelColumnBorderColor)
  Q_PROPERTY(QColor SelectedLevelColumnColor READ getSelectedLevelColumnColor
                 WRITE setSelectedLevelColumnColor)
  // PLI column
  QColor m_vectorColumnColor;          //(212,212,133)
  QColor m_vectorColumnBorderColor;    //(79,79,49)
  QColor m_selectedVectorColumnColor;  //(234,234,194)
  Q_PROPERTY(QColor VectorColumnColor READ getVectorColumnColor WRITE
                 setVectorColumnColor)
  Q_PROPERTY(QColor VectorColumnBorderColor READ getVectorColumnBorderColor
                 WRITE setVectorColumnBorderColor)
  Q_PROPERTY(QColor SelectedVectorColumnColor READ getSelectedVectorColumnColor
                 WRITE setSelectedVectorColumnColor)
  // subXsheet column
  QColor m_childColumnColor;          //(214,154,219)
  QColor m_childColumnBorderColor;    //(80,57,82)
  QColor m_selectedChildColumnColor;  //(235,205,237)
  Q_PROPERTY(QColor ChildColumnColor READ getChildColumnColor WRITE
                 setChildColumnColor)
  Q_PROPERTY(QColor ChildColumnBorderColor READ getChildColumnBorderColor WRITE
                 setChildColumnBorderColor)
  Q_PROPERTY(QColor SelectedChildColumnColor READ getSelectedChildColumnColor
                 WRITE setSelectedChildColumnColor)
  // Raster image column
  QColor m_fullcolorColumnColor;          //(154,214,219)
  QColor m_fullcolorColumnBorderColor;    //(57,80,82)
  QColor m_selectedFullcolorColumnColor;  //(205,235,237)
  Q_PROPERTY(QColor FullcolorColumnColor READ getFullcolorColumnColor WRITE
                 setFullcolorColumnColor)
  Q_PROPERTY(
      QColor FullcolorColumnBorderColor READ getFullcolorColumnBorderColor WRITE
          setFullcolorColumnBorderColor)
  Q_PROPERTY(
      QColor SelectedFullcolorColumnColor READ getSelectedFullcolorColumnColor
          WRITE setSelectedFullcolorColumnColor)
  // Fx column
  QColor m_fxColumnColor;          //(130,129,93)
  QColor m_fxColumnBorderColor;    //(48,48,35)
  QColor m_selectedFxColumnColor;  //(193,192,174)
  Q_PROPERTY(QColor FxColumnColor READ getFxColumnColor WRITE setFxColumnColor)
  Q_PROPERTY(QColor FxColumnBorderColor READ getFxColumnBorderColor WRITE
                 setFxColumnBorderColor)
  Q_PROPERTY(QColor SelectedFxColumnColor READ getSelectedFxColumnColor WRITE
                 setSelectedFxColumnColor)
  // Reference column
  QColor m_referenceColumnColor;          //(171,171,171)
  QColor m_referenceColumnBorderColor;    //(62,62,62)
  QColor m_selectedReferenceColumnColor;  //(213,213,213)
  Q_PROPERTY(QColor ReferenceColumnColor READ getReferenceColumnColor WRITE
                 setReferenceColumnColor)
  Q_PROPERTY(
      QColor ReferenceColumnBorderColor READ getReferenceColumnBorderColor WRITE
          setReferenceColumnBorderColor)
  Q_PROPERTY(
      QColor SelectedReferenceColumnColor READ getSelectedReferenceColumnColor
          WRITE setSelectedReferenceColumnColor)
  // Palette column
  QColor m_paletteColumnColor;          //(42,171,154)
  QColor m_paletteColumnBorderColor;    //(15,62,56)
  QColor m_selectedPaletteColumnColor;  //(146,221,202)
  Q_PROPERTY(QColor PaletteColumnColor READ getPaletteColumnColor WRITE
                 setPaletteColumnColor)
  Q_PROPERTY(QColor PaletteColumnBorderColor READ getPaletteColumnBorderColor
                 WRITE setPaletteColumnBorderColor)
  Q_PROPERTY(
      QColor SelectedPaletteColumnColor READ getSelectedPaletteColumnColor WRITE
          setSelectedPaletteColumnColor)
  // Mesh column
  QColor m_meshColumnColor;
  QColor m_meshColumnBorderColor;
  QColor m_selectedMeshColumnColor;
  Q_PROPERTY(
      QColor MeshColumnColor READ getMeshColumnColor WRITE setMeshColumnColor)
  Q_PROPERTY(QColor MeshColumnBorderColor READ getMeshColumnBorderColor WRITE
                 setMeshColumnBorderColor)
  Q_PROPERTY(QColor SelectedMeshColumnColor READ getSelectedMeshColumnColor
                 WRITE setSelectedMeshColumnColor)

  // Folder column
  QColor m_folderColumnColor;
  QColor m_folderColumnBorderColor;
  QColor m_selectedFolderColumnColor;
  Q_PROPERTY(
      QColor FolderColumnColor READ getFolderColumnColor WRITE setFolderColumnColor)
  Q_PROPERTY(QColor FolderColumnBorderColor READ getFolderColumnBorderColor WRITE
                 setFolderColumnBorderColor)
  Q_PROPERTY(QColor SelectedFolderColumnColor READ getSelectedFolderColumnColor
                 WRITE setSelectedFolderColumnColor)

  // Implicit Cell alpha
  int m_implicitCellAlpha;
  Q_PROPERTY(int ImplicitCellAlpha READ getImplicitCellAlpha WRITE
                 setImplicitCellAlpha);

  // Table color
  QColor m_tableColor;
  Q_PROPERTY(QColor TableColor READ getTableColor WRITE setTableColor)
  // Peg node
  QColor m_pegColor;
  Q_PROPERTY(QColor PegColor READ getPegColor WRITE setPegColor)
  // SoundText column
  QColor m_soundTextColumnColor;
  QColor m_soundTextColumnBorderColor;
  QColor m_selectedSoundTextColumnColor;
  Q_PROPERTY(QColor SoundTextColumnColor READ getSoundTextColumnColor WRITE
                 setSoundTextColumnColor)
  Q_PROPERTY(
      QColor SoundTextColumnBorderColor READ getSoundTextColumnBorderColor WRITE
          setSoundTextColumnBorderColor)
  Q_PROPERTY(
      QColor SelectedSoundTextColumnColor READ getSelectedSoundTextColumnColor
          WRITE setSelectedSoundTextColumnColor)
  // Sound column
  QColor m_soundColumnColor;
  QColor m_soundColumnBorderColor;
  QColor m_selectedSoundColumnColor;
  QColor m_soundColumnHlColor;
  QColor m_soundColumnTrackColor;
  Q_PROPERTY(QColor SoundColumnColor MEMBER m_soundColumnColor)
  Q_PROPERTY(QColor SoundColumnBorderColor MEMBER m_soundColumnBorderColor)
  Q_PROPERTY(QColor SelectedSoundColumnColor MEMBER m_selectedSoundColumnColor)
  Q_PROPERTY(QColor SoundColumnHlColor MEMBER m_soundColumnHlColor)
  Q_PROPERTY(QColor SoundColumnTrackColor MEMBER m_soundColumnTrackColor)

  // for making the column head lighter (255,255,255,50);
  QColor m_columnHeadPastelizer;
  Q_PROPERTY(QColor ColumnHeadPastelizer READ getColumnHeadPastelizer WRITE
                 setColumnHeadPastelizer)
  // selected column head (190,210,240,170);
  QColor m_selectedColumnHead;
  Q_PROPERTY(QColor SelectedColumnHead READ getSelectedColumnHead WRITE
                 setSelectedColumnHead)

  // For folded column
  QColor m_foldedColumnBGColor;
  QColor m_foldedColumnLineColor;
  Q_PROPERTY(QColor FoldedColumnBGColor READ getFoldedColumnBGColor WRITE
                 setFoldedColumnBGColor)
  Q_PROPERTY(QColor FoldedColumnLineColor READ getFoldedColumnLineColor WRITE
                 setFoldedColumnLineColor)

  // Xsheet Column name/Drag bar colors
  QColor m_xsheetColumnNameBgColor;
  QColor m_xsheetDragBarHighlightColor;
  Q_PROPERTY(QColor XsheetColumnNameBgColor READ getXsheetColumnNameBgColor
                 WRITE setXsheetColumnNameBgColor)
  Q_PROPERTY(
      QColor XsheetDragBarHighlightColor READ getXsheetDragBarHighlightColor
          WRITE setXsheetDragBarHighlightColor)

  // Xsheet Active Camera color
  QColor m_ActiveCameraColor;
  QColor m_SelectedActiveCameraColor;
  Q_PROPERTY(QColor ActiveCameraColor READ getActiveCameraColor WRITE
                 setActiveCameraColor)
  Q_PROPERTY(QColor SelectedActiveCameraColor READ getSelectedActiveCameraColor
                 WRITE setSelectedActiveCameraColor)
  // Xsheet Other Camera color
  QColor m_OtherCameraColor;
  QColor m_SelectedOtherCameraColor;
  Q_PROPERTY(QColor OtherCameraColor READ getOtherCameraColor WRITE
                 setOtherCameraColor)
  Q_PROPERTY(QColor SelectedOtherCameraColor READ getSelectedOtherCameraColor
                 WRITE setSelectedOtherCameraColor)

  // Xsheet Preview Button
  QColor m_xsheetPreviewButtonBgOnColor;
  QString m_xsheetPreviewButtonOnImage;
  QColor m_xsheetPreviewButtonBgOffColor;
  QString m_xsheetPreviewButtonOffImage;
  QString m_xsheetUnifiedButtonTranspImage;
  Q_PROPERTY(
      QColor XsheetPreviewButtonBgOnColor READ getXsheetPreviewButtonBgOnColor
          WRITE setXsheetPreviewButtonBgOnColor)
  Q_PROPERTY(
      QString XsheetPreviewButtonOnImage READ getXsheetPreviewButtonOnImage WRITE
          setXsheetPreviewButtonOnImage)
  Q_PROPERTY(
      QColor XsheetPreviewButtonBgOffColor READ getXsheetPreviewButtonBgOffColor
          WRITE setXsheetPreviewButtonBgOffColor)
  Q_PROPERTY(
      QString XsheetPreviewButtonOffImage READ getXsheetPreviewButtonOffImage
          WRITE setXsheetPreviewButtonOffImage)
  Q_PROPERTY(QString XsheetUnifiedButtonTranspImage READ
                 getXsheetUnifiedButtonTranspImage WRITE
                     setXsheetUnifiedButtonTranspImage)
  // Xsheet Camstand Button
  QColor m_xsheetCamstandButtonBgOnColor;
  QString m_xsheetCamstandButtonOnImage;
  QString m_xsheetCamstandButtonTranspImage;
  QColor m_xsheetCamstandButtonBgOffColor;
  QString m_xsheetCamstandButtonOffImage;
  Q_PROPERTY(
      QColor XsheetCamstandButtonBgOnColor READ getXsheetCamstandButtonBgOnColor
          WRITE setXsheetCamstandButtonBgOnColor)
  Q_PROPERTY(
      QString XsheetCamstandButtonOnImage READ getXsheetCamstandButtonOnImage
          WRITE setXsheetCamstandButtonOnImage)
  Q_PROPERTY(QString XsheetCamstandButtonTranspImage READ
                 getXsheetCamstandButtonTranspImage WRITE
                     setXsheetCamstandButtonTranspImage)
  Q_PROPERTY(QColor XsheetCamstandButtonBgOffColor READ
                 getXsheetCamstandButtonBgOffColor WRITE
                     setXsheetCamstandButtonBgOffColor)
  Q_PROPERTY(
      QString XsheetCamstandButtonOffImage READ getXsheetCamstandButtonOffImage
          WRITE setXsheetCamstandButtonOffImage)
  // Xsheet Lock Button
  QColor m_xsheetLockButtonBgOnColor;
  QString m_xsheetLockButtonOnImage;
  QColor m_xsheetLockButtonBgOffColor;
  QString m_xsheetLockButtonOffImage;
  Q_PROPERTY(QColor XsheetLockButtonBgOnColor READ getXsheetLockButtonBgOnColor
                 WRITE setXsheetLockButtonBgOnColor)
  Q_PROPERTY(QString XsheetLockButtonOnImage READ getXsheetLockButtonOnImage
                 WRITE setXsheetLockButtonOnImage)
  Q_PROPERTY(
      QColor XsheetLockButtonBgOffColor READ getXsheetLockButtonBgOffColor WRITE
          setXsheetLockButtonBgOffColor)
  Q_PROPERTY(QString XsheetLockButtonOffImage READ getXsheetLockButtonOffImage
                 WRITE setXsheetLockButtonOffImage)
  // Xsheet Config Button
  QColor m_xsheetConfigButtonBgColor;
  QString m_xsheetConfigButtonImage;
  Q_PROPERTY(QColor XsheetConfigButtonBgColor READ getXsheetConfigButtonBgColor
                 WRITE setXsheetConfigButtonBgColor)
  Q_PROPERTY(QString XsheetConfigButtonImage READ getXsheetConfigButtonImage
                 WRITE setXsheetConfigButtonImage)
  // Timeline Preview Button
  QColor m_timelinePreviewButtonBgOnColor;
  QString m_timelinePreviewButtonOnImage;
  QColor m_timelinePreviewButtonBgOffColor;
  QString m_timelinePreviewButtonOffImage;
  QString m_timelineUnifiedButtonTranspImage;
  Q_PROPERTY(QColor TimelinePreviewButtonBgOnColor READ
                 getTimelinePreviewButtonBgOnColor WRITE
                     setTimelinePreviewButtonBgOnColor)
  Q_PROPERTY(
      QString TimelinePreviewButtonOnImage READ getTimelinePreviewButtonOnImage
          WRITE setTimelinePreviewButtonOnImage)
  Q_PROPERTY(QColor TimelinePreviewButtonBgOffColor READ
                 getTimelinePreviewButtonBgOffColor WRITE
                     setTimelinePreviewButtonBgOffColor)
  Q_PROPERTY(
      QString TimelinePreviewButtonOffImage READ getTimelinePreviewButtonOffImage
          WRITE setTimelinePreviewButtonOffImage)
  Q_PROPERTY(QString TimelineUnifiedButtonTranspImage READ
                 getTimelineUnifiedButtonTranspImage WRITE
                     setTimelineUnifiedButtonTranspImage)
  // Timeline Camstand Button
  QColor m_timelineCamstandButtonBgOnColor;
  QString m_timelineCamstandButtonOnImage;
  QString m_timelineCamstandButtonTranspImage;
  QColor m_timelineCamstandButtonBgOffColor;
  QString m_timelineCamstandButtonOffImage;
  Q_PROPERTY(QColor TimelineCamstandButtonBgOnColor READ
                 getTimelineCamstandButtonBgOnColor WRITE
                     setTimelineCamstandButtonBgOnColor)
  Q_PROPERTY(
      QString TimelineCamstandButtonOnImage READ getTimelineCamstandButtonOnImage
          WRITE setTimelineCamstandButtonOnImage)
  Q_PROPERTY(QString TimelineCamstandButtonTranspImage READ
                 getTimelineCamstandButtonTranspImage WRITE
                     setTimelineCamstandButtonTranspImage)
  Q_PROPERTY(QColor TimelineCamstandButtonBgOffColor READ
                 getTimelineCamstandButtonBgOffColor WRITE
                     setTimelineCamstandButtonBgOffColor)
  Q_PROPERTY(QString TimelineCamstandButtonOffImage READ
                 getTimelineCamstandButtonOffImage WRITE
                     setTimelineCamstandButtonOffImage)
  // Timeline Lock Button
  QColor m_timelineLockButtonBgOnColor;
  QString m_timelineLockButtonOnImage;
  QColor m_timelineLockButtonBgOffColor;
  QString m_timelineLockButtonOffImage;
  Q_PROPERTY(
      QColor TimelineLockButtonBgOnColor READ getTimelineLockButtonBgOnColor
          WRITE setTimelineLockButtonBgOnColor)
  Q_PROPERTY(QString TimelineLockButtonOnImage READ getTimelineLockButtonOnImage
                 WRITE setTimelineLockButtonOnImage)
  Q_PROPERTY(
      QColor TimelineLockButtonBgOffColor READ getTimelineLockButtonBgOffColor
          WRITE setTimelineLockButtonBgOffColor)
  Q_PROPERTY(
      QString TimelineLockButtonOffImage READ getTimelineLockButtonOffImage WRITE
          setTimelineLockButtonOffImage)
  // Timeline Config Button
  QColor m_timelineConfigButtonBgColor;
  QString m_timelineConfigButtonImage;
  Q_PROPERTY(
      QColor TimelineConfigButtonBgColor READ getTimelineConfigButtonBgColor
          WRITE setTimelineConfigButtonBgColor)
  Q_PROPERTY(QString TimelineConfigButtonImage READ getTimelineConfigButtonImage
                 WRITE setTimelineConfigButtonImage)
  // Layer Header icons
  QString m_layerHeaderPreviewImage;
  QString m_layerHeaderPreviewOverImage;
  QString m_layerHeaderCamstandImage;
  QString m_layerHeaderCamstandOverImage;
  QString m_layerHeaderLockImage;
  QString m_layerHeaderLockOverImage;
  Q_PROPERTY(QString LayerHeaderPreviewImage READ getLayerHeaderPreviewImage
                 WRITE setLayerHeaderPreviewImage)
  Q_PROPERTY(
      QString LayerHeaderPreviewOverImage READ getLayerHeaderPreviewOverImage
          WRITE setLayerHeaderPreviewOverImage)
  Q_PROPERTY(QString LayerHeaderCamstandImage READ getLayerHeaderCamstandImage
                 WRITE setLayerHeaderCamstandImage)
  Q_PROPERTY(
      QString LayerHeaderCamstandOverImage READ getLayerHeaderCamstandOverImage
          WRITE setLayerHeaderCamstandOverImage)
  Q_PROPERTY(QString LayerHeaderLockImage READ getLayerHeaderLockImage WRITE
                 setLayerHeaderLockImage)
  Q_PROPERTY(QString LayerHeaderLockOverImage READ getLayerHeaderLockOverImage
                 WRITE setLayerHeaderLockOverImage)

  XsheetScrollArea *m_cellScrollArea;
  XsheetScrollArea *m_columnScrollArea;
  XsheetScrollArea *m_rowScrollArea;
  XsheetScrollArea *m_noteScrollArea;
  XsheetScrollArea *m_toolbarScrollArea;
  XsheetScrollArea *m_breadcrumbScrollArea;

  XsheetGUI::ColumnArea *m_columnArea;
  XsheetGUI::RowArea *m_rowArea;
  XsheetGUI::CellArea *m_cellArea;
  XsheetGUI::NoteArea *m_noteArea;
  XsheetGUI::QuickToolbar *m_toolbar;
  XsheetGUI::BreadcrumbArea *m_breadcrumbArea;

  LayerFooterPanel *m_layerFooterPanel;

  Spreadsheet::FrameScroller m_frameScroller;

  int m_timerId;
  QPoint m_autoPanSpeed;
  QPoint m_lastAutoPanPos;

  TColumnSelection *m_columnSelection;
  TCellKeyframeSelection *m_cellKeyframeSelection;

  int m_scrubCol, m_scrubRow0, m_scrubRow1;

  bool m_isCurrentFrameSwitched;
  bool m_isCurrentColumnSwitched;

  XsheetGUI::DragTool *m_dragTool;

  bool m_isComputingSize;

  QList<XsheetGUI::NoteWidget *> m_noteWidgets;
  int m_currentNoteIndex;

  Qt::KeyboardModifiers m_qtModifiers;

  const Orientation *m_orientation;

  QString m_xsheetLayout;

  int m_frameZoomFactor;

  CellPosition m_ctrlSelectRef;

  int m_xsheetBodyOffset;
  int m_timelineBodyOffset;

public:
  enum FrameDisplayStyle { Frame = 0, SecAndFrame, SixSecSheet, ThreeSecSheet };

private:
  FrameDisplayStyle m_frameDisplayStyle;

  FrameDisplayStyle to_enum(int n) {
    switch (n) {
    case 0:
      return Frame;
    case 1:
      return SecAndFrame;
    case 2:
      return SixSecSheet;
    default:
      return ThreeSecSheet;
    }
  }

public:
  XsheetViewer(QWidget *parent = 0, Qt::WindowFlags flags = Qt::WindowFlags());
  ~XsheetViewer();

  TColumnSelection *getColumnSelection() const { return m_columnSelection; }
  TCellSelection *getCellSelection() const {
    return m_cellKeyframeSelection->getCellSelection();
  }
  TKeyframeSelection *getKeyframeSelection() const {
    return m_cellKeyframeSelection->getKeyframeSelection();
  }
  TCellKeyframeSelection *getCellKeyframeSelection() const {
    return m_cellKeyframeSelection;
  }
  bool m_panningArmed = false;

  bool areCellsSelectedEmpty();
  /*! Return true if selection contain only sound cell.*/
  bool areSoundCellsSelected();
  bool areSoundTextCellsSelected();
  bool areFolderCellsSelected();
  bool areCameraCellsSelected();

  XsheetGUI::DragTool *getDragTool() const { return m_dragTool; };
  void setDragTool(XsheetGUI::DragTool *dragTool);

  void dragToolClick(QMouseEvent *);
  void dragToolDrag(QMouseEvent *);
  void dragToolRelease(QMouseEvent *);  // n.b. cancella il dragtool

  void dragToolLeave(QEvent *);  // n.b. cancella il dragtool

  void dragToolClick(QDropEvent *);
  void dragToolDrag(QDropEvent *);
  void dragToolRelease(QDropEvent *);  // n.b. cancella il dragtool

  void setQtModifiers(Qt::KeyboardModifiers value) { m_qtModifiers = value; }

  void setScrubHighlight(int row, int starRow, int col);
  void resetScrubHighlight();
  void getScrubHeighlight(int &R0, int &R1);
  bool isScrubHighlighted(int row, int col);

  TXsheet *getXsheet() const;
  int getCurrentColumn() const;
  int getMenuColumnTarget() const;
  int getCurrentRow() const;
  //! Restituisce la \b objectId corrispondente alla colonna \b col
  TStageObjectId getObjectId(int col) const;

  void setCurrentColumn(int col);
  void setCurrentRow(int row);

  void scroll(QPoint delta);
  int getColumnScrollValue();

  void setAutoPanSpeed(const QPoint &speed);
  void setAutoPanSpeed(const QRect &widgetBounds, const QPoint &mousePos);
  void stopAutoPan() { setAutoPanSpeed(QPoint()); }
  bool isAutoPanning() const {
    return m_autoPanSpeed.x() != 0 || m_autoPanSpeed.y() != 0;
  }

  //-------
  const Orientation *orientation() const;
  void flipOrientation();

  CellPosition xyToPosition(const QPoint &point) const;
  CellPosition xyToPosition(const TPoint &point) const;
  CellPosition xyToPosition(const TPointD &point) const;
  QPoint positionToXY(const CellPosition &pos) const;

  int colToTimelineLayerAxis(int layer) const;

  int columnToLayerAxis(int layer) const;
  int rowToFrameAxis(int frame) const;

  CellRange xyRectToRange(const QRect &rect) const;
  QRect rangeToXYRect(const CellRange &range) const;

  void drawPredefinedPath(QPainter &p, PredefinedPath which,
                          const CellPosition &pos, optional<QColor> fill,
                          optional<QColor> outline, int lineWidth = 1) const;

  void drawPredefinedPath(QPainter &p, PredefinedPath which, QPoint xy,
                          optional<QColor> fill, optional<QColor> outline,
                          int lineWidth = 1) const;

  //---------

  void updateCells() { m_cellArea->update(m_cellArea->visibleRegion()); }
  void updateRows() { m_rowArea->update(m_rowArea->visibleRegion()); }
  void updateColumns() { m_columnArea->update(m_columnArea->visibleRegion()); }
  bool refreshContentSize(int scrollDx, int scrollDy);

  void updateAreeSize();

  QList<XsheetGUI::NoteWidget *> getNotesWidget() const;
  void addNoteWidget(XsheetGUI::NoteWidget *w);
  int getCurrentNoteIndex() const;
  //! Clear notes widgets std::vector.
  void clearNoteWidgets();
  //! Update notes widgets and update cell.
  void updateNoteWidgets();
  //! Discard Note Widget
  void discardNoteWidget();

  void setCurrentNoteIndex(int currentNoteIndex);

  void toggleCurrentFolderOpenClose();

  // scroll the cell area to make a cell at (row,col) visible
  void scrollTo(int row, int col);

  // QProperty
  void setLightLightBGColor(const QColor &color) {
    m_lightLightBgColor = color;
  }
  QColor getLightLightBGColor() const { return m_lightLightBgColor; }
  void setLightBGColor(const QColor &color) { m_lightBgColor = color; }
  QColor getLightBGColor() const { return m_lightBgColor; }
  void setBGColor(const QColor &color) { m_bgColor = color; }
  QColor getBGColor() const { return m_bgColor; }
  void setDarkBGColor(const QColor &color) { m_darkBgColor = color; }
  QColor getDarkBGColor() const { return m_darkBgColor; }
  void setLightLineColor(const QColor &color) { m_lightLineColor = color; }
  QColor getLightLineColor() const { return m_lightLineColor; }
  void setDarkLineColor(const QColor &color) { m_darkLineColor = color; }
  QColor getDarkLineColor() const { return m_darkLineColor; }

  QColor getColumnIconLineColor() const { return m_columnIconLineColor; }
  void setColumnIconLineColor(const QColor &color) {
    m_columnIconLineColor = color;
  }
  QColor getTimelineIconLineColor() const { return m_timelineIconLineColor; }
  void setTimelineIconLineColor(const QColor &color) {
    m_timelineIconLineColor = color;
  }
  QColor getFrameRangeMarkerLineColor() const {
    return m_frameRangeMarkerLineColor;
  }
  void setFrameRangeMarkerLineColor(const QColor &color) {
    m_frameRangeMarkerLineColor = color;
  }

  // Row
  void setCurrentRowBgColor(const QColor &color) {
    m_currentRowBgColor = color;
  }
  QColor getCurrentRowBgColor() const { return m_currentRowBgColor; }
  void setMarkerLineColor(const QColor &color) { m_markerLineColor = color; }
  QColor getMarkerLineColor() const { return m_markerLineColor; }
  void setSecMarkerLineColor(const QColor &color) {
    m_secMarkerLineColor = color;
  }
  QColor getSecMarkerLineColor() const { return m_secMarkerLineColor; }
  void setSelectedMarkerLineColor(const QColor &color) {
    m_selectedMarkerLineColor = color;
  }
  QColor getSelectedMarkerLineColor() const {
    return m_selectedMarkerLineColor;
  }
  void setVerticalLineColor(const QColor &color) {
    m_verticalLineColor = color;
  }
  QColor getVerticalLineColor() const { return m_verticalLineColor; }
  void setVerticalLineHeadColor(const QColor &color) {
    m_verticalLineHeadColor = color;
  }
  QColor getVerticalLineHeadColor() const { return m_verticalLineHeadColor; }
  void setTextColor(const QColor &color) { m_textColor = color; }
  QColor getTextColor() const { return m_textColor; }
  void setErrorTextColor(const QColor &color) { m_errorTextColor = color; }
  QColor getErrorTextColor() const { return m_errorTextColor; }
  void setSelectedTextColor(const QColor &color) {
    m_selectedTextColor = color;
  }
  QColor getSelectedTextColor() const { return m_selectedTextColor; }
  void setFrameTextColor(const QColor &color) {
    m_frameTextColor = color;
  }
  QColor getFrameTextColor() const { return m_frameTextColor; }
  void setCurrentFrameTextColor(const QColor &color) {
    m_currentFrameTextColor = color;
  }
  QColor getKeyframeLineColor() const { return m_keyframeLineColor; }
  void setKeyframeLineColor(const QColor &color) {
    m_keyframeLineColor = color;
  }
  QColor getCurrentFrameTextColor() const { return m_currentFrameTextColor; }
  void setPreviewFrameTextColor(const QColor &color) {
    m_previewFrameTextColor = color;
  }
  QColor getPreviewFrameTextColor() const { return m_previewFrameTextColor; }
  void setOnionSkinAreaBgColor(const QColor &color) {
    m_onionSkinAreaBgColor = color;
  }
  QColor getOnionSkinAreaBgColor() const { return m_onionSkinAreaBgColor; }

  // Column
  void setColumnTextColor(const QColor &color) {
    m_columnTextColor = color;
  }
  QColor getColumnTextColor() const {
    return m_columnTextColor;
  }
  void setHighlightColumnTextColor(const QColor &color) {
    m_highlightColumnTextColor = color;
  }
  QColor getHighlightColumnTextColor() const {
    return m_highlightColumnTextColor;
  }
  void setEmptyColumnHeadColor(const QColor &color) {
    m_emptyColumnHeadColor = color;
  }
  QColor getEmptyColumnHeadColor() const { return m_emptyColumnHeadColor; }

  // specified by preferences
  QColor getColumnFocusColor() const;

  // Cell
  void setEmptyCellColor(const QColor &color) { m_emptyCellColor = color; }
  QColor getEmptyCellColor() const { return m_emptyCellColor; }
  void setNotEmptyColumnColor(const QColor &color) {
    m_notEmptyColumnColor = color;
  }
  QColor getNotEmptyColumnColor() const { return m_notEmptyColumnColor; }
  void setSelectedEmptyCellColor(const QColor &color) {
    m_selectedEmptyCellColor = color;
  }
  QColor getSelectedEmptyCellColor() const { return m_selectedEmptyCellColor; }
  void setLevelEndColor(const QColor &color) { m_levelEndColor = color; }
  QColor getLevelEndColor() const { return m_levelEndColor; }

  // Cell focus
  //  void setCellFocusColor(const QColor &color) { m_cellFocusColor = color; }
  QColor getCellFocusColor() const;  // { return m_cellFocusColor; }

  // Play range
  QColor getPlayRangeColor() const { return m_playRangeColor; }
  void setPlayRangeColor(const QColor &color) { m_playRangeColor = color; }

  // TZP column
  void setLevelColumnColor(const QColor &color) { m_levelColumnColor = color; }
  void setLevelColumnBorderColor(const QColor &color) {
    m_levelColumnBorderColor = color;
  }
  void setSelectedLevelColumnColor(const QColor &color) {
    m_selectedLevelColumnColor = color;
  }
  QColor getLevelColumnColor() const { return m_levelColumnColor; }
  QColor getLevelColumnBorderColor() const { return m_levelColumnBorderColor; }
  QColor getSelectedLevelColumnColor() const {
    return m_selectedLevelColumnColor;
  }
  // PLI column
  void setVectorColumnColor(const QColor &color) {
    m_vectorColumnColor = color;
  }
  void setVectorColumnBorderColor(const QColor &color) {
    m_vectorColumnBorderColor = color;
  }
  void setSelectedVectorColumnColor(const QColor &color) {
    m_selectedVectorColumnColor = color;
  }
  QColor getVectorColumnColor() const { return m_vectorColumnColor; }
  QColor getVectorColumnBorderColor() const {
    return m_vectorColumnBorderColor;
  }
  QColor getSelectedVectorColumnColor() const {
    return m_selectedVectorColumnColor;
  }
  // subXsheet column
  void setChildColumnColor(const QColor &color) { m_childColumnColor = color; }
  void setChildColumnBorderColor(const QColor &color) {
    m_childColumnBorderColor = color;
  }
  void setSelectedChildColumnColor(const QColor &color) {
    m_selectedChildColumnColor = color;
  }
  QColor getChildColumnColor() const { return m_childColumnColor; }
  QColor getChildColumnBorderColor() const { return m_childColumnBorderColor; }
  QColor getSelectedChildColumnColor() const {
    return m_selectedChildColumnColor;
  }
  // Raster image column
  void setFullcolorColumnColor(const QColor &color) {
    m_fullcolorColumnColor = color;
  }
  void setFullcolorColumnBorderColor(const QColor &color) {
    m_fullcolorColumnBorderColor = color;
  }
  void setSelectedFullcolorColumnColor(const QColor &color) {
    m_selectedFullcolorColumnColor = color;
  }
  QColor getFullcolorColumnColor() const { return m_fullcolorColumnColor; }
  QColor getFullcolorColumnBorderColor() const {
    return m_fullcolorColumnBorderColor;
  }
  QColor getSelectedFullcolorColumnColor() const {
    return m_selectedFullcolorColumnColor;
  }
  // Fx column
  void setFxColumnColor(const QColor &color) { m_fxColumnColor = color; }
  void setFxColumnBorderColor(const QColor &color) {
    m_fxColumnBorderColor = color;
  }
  void setSelectedFxColumnColor(const QColor &color) {
    m_selectedFxColumnColor = color;
  }
  QColor getFxColumnColor() const { return m_fxColumnColor; }
  QColor getFxColumnBorderColor() const { return m_fxColumnBorderColor; }
  QColor getSelectedFxColumnColor() const { return m_selectedFxColumnColor; }
  // Reference column
  void setReferenceColumnColor(const QColor &color) {
    m_referenceColumnColor = color;
  }
  void setReferenceColumnBorderColor(const QColor &color) {
    m_referenceColumnBorderColor = color;
  }
  void setSelectedReferenceColumnColor(const QColor &color) {
    m_selectedReferenceColumnColor = color;
  }
  QColor getReferenceColumnColor() const { return m_referenceColumnColor; }
  QColor getReferenceColumnBorderColor() const {
    return m_referenceColumnBorderColor;
  }
  QColor getSelectedReferenceColumnColor() const {
    return m_selectedReferenceColumnColor;
  }
  // Palette column
  void setPaletteColumnColor(const QColor &color) {
    m_paletteColumnColor = color;
  }
  void setPaletteColumnBorderColor(const QColor &color) {
    m_paletteColumnBorderColor = color;
  }
  void setSelectedPaletteColumnColor(const QColor &color) {
    m_selectedPaletteColumnColor = color;
  }
  QColor getPaletteColumnColor() const { return m_paletteColumnColor; }
  QColor getPaletteColumnBorderColor() const {
    return m_paletteColumnBorderColor;
  }
  QColor getSelectedPaletteColumnColor() const {
    return m_selectedPaletteColumnColor;
  }
  // Mesh column
  void setMeshColumnColor(const QColor &color) { m_meshColumnColor = color; }
  void setMeshColumnBorderColor(const QColor &color) {
    m_meshColumnBorderColor = color;
  }
  void setSelectedMeshColumnColor(const QColor &color) {
    m_selectedMeshColumnColor = color;
  }
  QColor getMeshColumnColor() const { return m_meshColumnColor; }
  QColor getMeshColumnBorderColor() const { return m_meshColumnBorderColor; }
  QColor getSelectedMeshColumnColor() const {
    return m_selectedMeshColumnColor;
  }
  // Folder column
  void setFolderColumnColor(const QColor &color) {
    m_folderColumnColor = color;
  }
  void setFolderColumnBorderColor(const QColor &color) {
    m_folderColumnBorderColor = color;
  }
  void setSelectedFolderColumnColor(const QColor &color) {
    m_selectedFolderColumnColor = color;
  }
  QColor getFolderColumnColor() const { return m_folderColumnColor; }
  QColor getFolderColumnBorderColor() const {
    return m_folderColumnBorderColor;
  }
  QColor getSelectedFolderColumnColor() const {
    return m_selectedFolderColumnColor;
  }

  // Implicit Cell Alpha
  void setImplicitCellAlpha(const int &alpha) { m_implicitCellAlpha = alpha; }
  int getImplicitCellAlpha() const { return m_implicitCellAlpha; }

  // Table node
  void setTableColor(const QColor &color) { m_tableColor = color; }
  QColor getTableColor() const { return m_tableColor; }
  // Peg node
  void setPegColor(const QColor &color) { m_pegColor = color; }
  QColor getPegColor() const { return m_pegColor; }
  // SoundText column
  void setSoundTextColumnColor(const QColor &color) {
    m_soundTextColumnColor = color;
  }
  void setSoundTextColumnBorderColor(const QColor &color) {
    m_soundTextColumnBorderColor = color;
  }
  void setSelectedSoundTextColumnColor(const QColor &color) {
    m_selectedSoundTextColumnColor = color;
  }
  QColor getSoundTextColumnColor() const { return m_soundTextColumnColor; }
  QColor getSoundTextColumnBorderColor() const {
    return m_soundTextColumnBorderColor;
  }
  QColor getSelectedSoundTextColumnColor() const {
    return m_selectedSoundTextColumnColor;
  }
  // Sound column
  QColor getSoundColumnHlColor() const { return m_soundColumnHlColor; }
  QColor getSoundColumnTrackColor() const { return m_soundColumnTrackColor; }

  void setColumnHeadPastelizer(const QColor &color) {
    m_columnHeadPastelizer = color;
  }
  QColor getColumnHeadPastelizer() const { return m_columnHeadPastelizer; }
  void setSelectedColumnHead(const QColor &color) {
    m_selectedColumnHead = color;
  }
  QColor getSelectedColumnHead() const { return m_selectedColumnHead; }

  void getCellTypeAndColors(int &ltype, QColor &cellColor, QColor &sideColor,
                            const TXshCell &cell, bool isSelected = false);
  void getColumnColor(QColor &color, QColor &sidecolor, int index,
                      TXsheet *xsh);

  // For folded column
  QColor getFoldedColumnBGColor() const { return m_foldedColumnBGColor; }
  QColor getFoldedColumnLineColor() const { return m_foldedColumnLineColor; }
  void setFoldedColumnBGColor(const QColor &color) {
    m_foldedColumnBGColor = color;
  }
  void setFoldedColumnLineColor(const QColor &color) {
    m_foldedColumnLineColor = color;
  }
  // Xsheet Column Name/Drag Bar
  void setXsheetColumnNameBgColor(const QColor &color) {
    m_xsheetColumnNameBgColor = color;
  }
  void setXsheetDragBarHighlightColor(const QColor &color) {
    m_xsheetDragBarHighlightColor = color;
  }
  QColor getXsheetColumnNameBgColor() const {
    return m_xsheetColumnNameBgColor;
  }
  QColor getXsheetDragBarHighlightColor() const {
    return m_xsheetDragBarHighlightColor;
  }

  // Xsheet Active Camera Color
  void setActiveCameraColor(const QColor &color) {
    m_ActiveCameraColor = color;
  }
  void setSelectedActiveCameraColor(const QColor &color) {
    m_SelectedActiveCameraColor = color;
  }
  QColor getActiveCameraColor() const { return m_ActiveCameraColor; }
  QColor getSelectedActiveCameraColor() const {
    return m_SelectedActiveCameraColor;
  }
  // Xsheet Other Camera Color
  void setOtherCameraColor(const QColor &color) { m_OtherCameraColor = color; }
  void setSelectedOtherCameraColor(const QColor &color) {
    m_SelectedOtherCameraColor = color;
  }
  QColor getOtherCameraColor() const { return m_OtherCameraColor; }
  QColor getSelectedOtherCameraColor() const {
    return m_SelectedOtherCameraColor;
  }

  // Xsheet Preview Button
  void setXsheetPreviewButtonBgOnColor(const QColor &color) {
    m_xsheetPreviewButtonBgOnColor = color;
  }
  void setXsheetPreviewButtonOnImage(const QString &svgFIlePath) {
    m_xsheetPreviewButtonOnImage = svgFIlePath;
  }
  void setXsheetPreviewButtonBgOffColor(const QColor &color) {
    m_xsheetPreviewButtonBgOffColor = color;
  }
  void setXsheetPreviewButtonOffImage(const QString &svgFIlePath) {
    m_xsheetPreviewButtonOffImage = svgFIlePath;
  }
  void setXsheetUnifiedButtonTranspImage(const QString &svgFIlePath) {
    m_xsheetUnifiedButtonTranspImage = svgFIlePath;
  }
  QColor getXsheetPreviewButtonBgOnColor() const {
    return m_xsheetPreviewButtonBgOnColor;
  }
  QString getXsheetPreviewButtonOnImage() const {
    return m_xsheetPreviewButtonOnImage;
  }
  QColor getXsheetPreviewButtonBgOffColor() const {
    return m_xsheetPreviewButtonBgOffColor;
  }
  QString getXsheetPreviewButtonOffImage() const {
    return m_xsheetPreviewButtonOffImage;
  }
  QString getXsheetUnifiedButtonTranspImage() const {
    return m_xsheetUnifiedButtonTranspImage;
  }
  // Xsheet Camstand Button
  void setXsheetCamstandButtonBgOnColor(const QColor &color) {
    m_xsheetCamstandButtonBgOnColor = color;
  }
  void setXsheetCamstandButtonOnImage(const QString &svgFIlePath) {
    m_xsheetCamstandButtonOnImage = svgFIlePath;
  }
  void setXsheetCamstandButtonTranspImage(const QString &svgFIlePath) {
    m_xsheetCamstandButtonTranspImage = svgFIlePath;
  }
  void setXsheetCamstandButtonBgOffColor(const QColor &color) {
    m_xsheetCamstandButtonBgOffColor = color;
  }
  void setXsheetCamstandButtonOffImage(const QString &svgFIlePath) {
    m_xsheetCamstandButtonOffImage = svgFIlePath;
  }
  QColor getXsheetCamstandButtonBgOnColor() const {
    return m_xsheetCamstandButtonBgOnColor;
  }
  QString getXsheetCamstandButtonOnImage() const {
    return m_xsheetCamstandButtonOnImage;
  }
  QString getXsheetCamstandButtonTranspImage() const {
    return m_xsheetCamstandButtonTranspImage;
  }
  QColor getXsheetCamstandButtonBgOffColor() const {
    return m_xsheetCamstandButtonBgOffColor;
  }
  QString getXsheetCamstandButtonOffImage() const {
    return m_xsheetCamstandButtonOffImage;
  }
  // XsheetLock Button
  void setXsheetLockButtonBgOnColor(const QColor &color) {
    m_xsheetLockButtonBgOnColor = color;
  }
  void setXsheetLockButtonOnImage(const QString &svgFIlePath) {
    m_xsheetLockButtonOnImage = svgFIlePath;
  }
  void setXsheetLockButtonBgOffColor(const QColor &color) {
    m_xsheetLockButtonBgOffColor = color;
  }
  void setXsheetLockButtonOffImage(const QString &svgFIlePath) {
    m_xsheetLockButtonOffImage = svgFIlePath;
  }
  QColor getXsheetLockButtonBgOnColor() const {
    return m_xsheetLockButtonBgOnColor;
  }
  QString getXsheetLockButtonOnImage() const {
    return m_xsheetLockButtonOnImage;
  }
  QColor getXsheetLockButtonBgOffColor() const {
    return m_xsheetLockButtonBgOffColor;
  }
  QString getXsheetLockButtonOffImage() const {
    return m_xsheetLockButtonOffImage;
  }
  // Xsheet Config Button
  void setXsheetConfigButtonBgColor(const QColor &color) {
    m_xsheetConfigButtonBgColor = color;
  }
  void setXsheetConfigButtonImage(const QString &svgFIlePath) {
    m_xsheetConfigButtonImage = svgFIlePath;
  }
  QColor getXsheetConfigButtonBgColor() const {
    return m_xsheetConfigButtonBgColor;
  }
  QString getXsheetConfigButtonImage() const {
    return m_xsheetConfigButtonImage;
  }
  // Timeline Preview Button
  void setTimelinePreviewButtonBgOnColor(const QColor &color) {
    m_timelinePreviewButtonBgOnColor = color;
  }
  void setTimelinePreviewButtonOnImage(const QString &svgFIlePath) {
    m_timelinePreviewButtonOnImage = svgFIlePath;
  }
  void setTimelinePreviewButtonBgOffColor(const QColor &color) {
    m_timelinePreviewButtonBgOffColor = color;
  }
  void setTimelinePreviewButtonOffImage(const QString &svgFIlePath) {
    m_timelinePreviewButtonOffImage = svgFIlePath;
  }
  void setTimelineUnifiedButtonTranspImage(const QString &svgFIlePath) {
    m_timelineUnifiedButtonTranspImage = svgFIlePath;
  }
  QColor getTimelinePreviewButtonBgOnColor() const {
    return m_timelinePreviewButtonBgOnColor;
  }
  QString getTimelinePreviewButtonOnImage() const {
    return m_timelinePreviewButtonOnImage;
  }
  QColor getTimelinePreviewButtonBgOffColor() const {
    return m_timelinePreviewButtonBgOffColor;
  }
  QString getTimelinePreviewButtonOffImage() const {
    return m_timelinePreviewButtonOffImage;
  }
  QString getTimelineUnifiedButtonTranspImage() const {
    return m_timelineUnifiedButtonTranspImage;
  }
  // Timeline Camstand Button
  void setTimelineCamstandButtonBgOnColor(const QColor &color) {
    m_timelineCamstandButtonBgOnColor = color;
  }
  void setTimelineCamstandButtonOnImage(const QString &svgFIlePath) {
    m_timelineCamstandButtonOnImage = svgFIlePath;
  }
  void setTimelineCamstandButtonTranspImage(const QString &svgFIlePath) {
    m_timelineCamstandButtonTranspImage = svgFIlePath;
  }
  void setTimelineCamstandButtonBgOffColor(const QColor &color) {
    m_timelineCamstandButtonBgOffColor = color;
  }
  void setTimelineCamstandButtonOffImage(const QString &svgFIlePath) {
    m_timelineCamstandButtonOffImage = svgFIlePath;
  }
  QColor getTimelineCamstandButtonBgOnColor() const {
    return m_timelineCamstandButtonBgOnColor;
  }
  QString getTimelineCamstandButtonOnImage() const {
    return m_timelineCamstandButtonOnImage;
  }
  QString getTimelineCamstandButtonTranspImage() const {
    return m_timelineCamstandButtonTranspImage;
  }
  QColor getTimelineCamstandButtonBgOffColor() const {
    return m_timelineCamstandButtonBgOffColor;
  }
  QString getTimelineCamstandButtonOffImage() const {
    return m_timelineCamstandButtonOffImage;
  }
  // Timeline Lock Button
  void setTimelineLockButtonBgOnColor(const QColor &color) {
    m_timelineLockButtonBgOnColor = color;
  }
  void setTimelineLockButtonOnImage(const QString &svgFIlePath) {
    m_timelineLockButtonOnImage = svgFIlePath;
  }
  void setTimelineLockButtonBgOffColor(const QColor &color) {
    m_timelineLockButtonBgOffColor = color;
  }
  void setTimelineLockButtonOffImage(const QString &svgFIlePath) {
    m_timelineLockButtonOffImage = svgFIlePath;
  }
  QColor getTimelineLockButtonBgOnColor() const {
    return m_timelineLockButtonBgOnColor;
  }
  QString getTimelineLockButtonOnImage() const {
    return m_timelineLockButtonOnImage;
  }
  QColor getTimelineLockButtonBgOffColor() const {
    return m_timelineLockButtonBgOffColor;
  }
  QString getTimelineLockButtonOffImage() const {
    return m_timelineLockButtonOffImage;
  }
  // Timeline Config Button
  void setTimelineConfigButtonBgColor(const QColor &color) {
    m_timelineConfigButtonBgColor = color;
  }
  void setTimelineConfigButtonImage(const QString &svgFIlePath) {
    m_timelineConfigButtonImage = svgFIlePath;
  }
  QColor getTimelineConfigButtonBgColor() const {
    return m_timelineConfigButtonBgColor;
  }
  QString getTimelineConfigButtonImage() const {
    return m_timelineConfigButtonImage;
  }
  // Layer Header icons
  void setLayerHeaderPreviewImage(const QString &svgFIlePath) {
    m_layerHeaderPreviewImage = svgFIlePath;
  }
  void setLayerHeaderPreviewOverImage(const QString &svgFIlePath) {
    m_layerHeaderPreviewOverImage = svgFIlePath;
  }
  void setLayerHeaderCamstandImage(const QString &svgFIlePath) {
    m_layerHeaderCamstandImage = svgFIlePath;
  }
  void setLayerHeaderCamstandOverImage(const QString &svgFIlePath) {
    m_layerHeaderCamstandOverImage = svgFIlePath;
  }
  void setLayerHeaderLockImage(const QString &svgFIlePath) {
    m_layerHeaderLockImage = svgFIlePath;
  }
  void setLayerHeaderLockOverImage(const QString &svgFIlePath) {
    m_layerHeaderLockOverImage = svgFIlePath;
  }
  QString getLayerHeaderPreviewImage() const {
    return m_layerHeaderPreviewImage;
  }
  QString getLayerHeaderPreviewOverImage() const {
    return m_layerHeaderPreviewOverImage;
  }
  QString getLayerHeaderCamstandImage() const {
    return m_layerHeaderCamstandImage;
  }
  QString getLayerHeaderCamstandOverImage() const {
    return m_layerHeaderCamstandOverImage;
  }
  QString getLayerHeaderLockImage() const { return m_layerHeaderLockImage; }
  QString getLayerHeaderLockOverImage() const {
    return m_layerHeaderLockOverImage;
  }

  void getButton(const int &btype, QColor &bgColor, QString &svgIconPath,
                 bool isTimeline = false);

  // convert the last one digit of the frame number to alphabet
  // Ex.  12 -> 1B    21 -> 2A   30 -> 3
  QString getFrameNumberWithLetters(int frame);

  void setFrameDisplayStyle(FrameDisplayStyle style);
  FrameDisplayStyle getFrameDisplayStyle() { return m_frameDisplayStyle; }

  // SaveLoadQSettings
  virtual void save(QSettings &settings,
                    bool forPopupIni = false) const override;
  virtual void load(QSettings &settings) override;

  QString getXsheetLayout() const { return m_xsheetLayout; }
  // returns a list of frame amount per page displayable in the current size
  QList<int> availableFramesPerPage();
  void zoomToFramesPerPage(int frames);

  int getContextMenuRow() { return m_rowArea->getContextMenuRow(); }

  int getXsheetBodyOffset() const { return m_xsheetBodyOffset; }
  void setXsheetBodyOffset(int offset) { m_xsheetBodyOffset = offset; };
  int getTimelineBodyOffset() const { return m_timelineBodyOffset; }
  void setTimelineBodyOffset(int offset) { m_timelineBodyOffset = offset; };

protected:
  void scrollToColumn(int col);
  void scrollToHorizontalRange(int x0, int x1);
  void scrollToRow(int row);
  void scrollToVerticalRange(int y0, int y1);

  void showEvent(QShowEvent *) override;
  void hideEvent(QHideEvent *) override;
  void resizeEvent(QResizeEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  // display the upper-directional smart tab only when the ctrl key is pressed
  void keyReleaseEvent(QKeyEvent *event) override;
  void enterEvent(QEvent *) override;
  void leaveEvent(QEvent *) override;
  void wheelEvent(QWheelEvent *event) override;
  void timerEvent(QTimerEvent *) override;
  bool event(QEvent *) override;

  void disconnectScrollBars();
  void connectScrollBars();
  void connectOrDisconnectScrollBars(bool toConnect);
  void connectOrDisconnect(bool toConnect, QWidget *sender, const char *signal,
                           QWidget *receiver, const char *slot);
signals:
  void orientationChanged(const Orientation *newOrientation);

public slots:
  void positionSections();
  void onSceneSwitched();
  void onXsheetChanged();
  void onCurrentFrameSwitched();
  void onPlayingStatusChanged();
  void onCurrentColumnSwitched();
  void onSelectionSwitched(TSelection *oldSelection, TSelection *newSelection);

  void onSelectionChanged(TSelection *selection);

  void updateAllAree(bool isDragging = false);
  void updateColumnArea();
  void updateCellColumnAree();
  void updateCellRowAree();

  void onScrubStopped();
  void onPreferenceChanged(const QString &prefName);
  //! Aggiorna il "titolo" del widget.
  void changeWindowTitle();

  void resetXsheetNotes();

  void onOrientationChanged(const Orientation *newOrientation);
  void onPrepareToScrollOffset(const QPointF &offset);
  void onZoomScrollAdjust(QPointF &offset, bool toZoom);

  void setFrameZoomFactor(int f) { m_frameZoomFactor = f; }
  int getFrameZoomFactor() const;
  QPoint getFrameZoomAdjustment();

  void zoomOnFrame(int frame, int factor);
};

#endif  // XSHEETVIEWER_H

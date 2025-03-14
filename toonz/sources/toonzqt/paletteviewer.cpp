

#include "toonzqt/paletteviewer.h"

// TnzQt includes
#include "toonzqt/gutil.h"
#include "toonzqt/keyframenavigator.h"
#include "toonzqt/trepetitionguard.h"
#include "toonzqt/dvdialog.h"
#include "toonzqt/dvscrollwidget.h"
#include "toonzqt/studiopaletteviewer.h"
#include "toonzqt/styleselection.h"
#include "toonzqt/stylenameeditor.h"
#include "palettedata.h"

// TnzLib includes
#include "toonz/palettecmd.h"
#include "toonz/studiopalettecmd.h"
#include "toonz/txshlevel.h"
#include "toonz/toonzscene.h"
#include "toonz/sceneproperties.h"
#include "toonz/studiopalette.h"
#include "toonz/tframehandle.h"
#include "toonz/fullcolorpalette.h"
#include "toonz/txshlevelhandle.h"
#include "toonz/txshleveltypes.h"

#include "../include/toonz/palettecontroller.h"

// TnzCore includes
#include "tconvert.h"
#include "tsystem.h"
#include "tenv.h"

// Qt includes
#include <QVBoxLayout>
#include <QToolBar>
#include <QScrollArea>
#include <QMouseEvent>
#include <QMenuBar>
#include <QToolButton>
#include <QUrl>
#include <QApplication>
#include <QLabel>
#include <QDrag>

TEnv::IntVar ShowNewStyleButton("ShowNewStyleButton", 1);
using namespace PaletteViewerGUI;

//=============================================================================
/*! \class PaletteViewer
\brief The PaletteViewer class provides an object to view and
manage palette view.

Inherits \b QWidget.

This object allows to show and manage palette; it's possible
distinguish three
view type, class can show: current level palette, current studio
palette or
current cleanup palette, using setPalette() to set palette
viewer.
The object is composed of a vertical layout with three widget:
\li a tab bar \b PaletteViewerGUI::TabBar in which are displayed
palette page
\b createTabBar();
\li a central frame \b PaletteViewerGUI::PageViewer in which are
displayed
all style of current page in palette
\li a button bar, \b createToolBar(), \b createPaletteToolBar()
and \b createSavePaletteToolBar();

A collection of method allows you to manage this object and its
interaction
with palette.
*/
/*!	\fn const TPaletteP PaletteViewer::&getPalette() const
Return current viewer palette.
*/
/*!	\fn void PaletteViewer::createToolBar()
Create down button bar.
*/
/*!	\fn void PaletteViewer::updateToolBar()
Update button bar.
*/
PaletteViewer::PaletteViewer(QWidget *parent, PaletteViewType viewType,
                             bool hasSaveToolBar, bool hasPageCommand,
                             bool hasPasteColors)
    : QFrame(parent)
    , m_tabBarContainer(0)
    , m_pagesBar(0)
    , m_paletteToolBar(0)
    , m_savePaletteToolBar(0)
    , m_pageViewer(0)
    , m_pageViewerScrollArea(0)
    , m_indexPageToDelete(-1)
    , m_viewType(viewType)
    , m_frameHandle(0)
    , m_levelHandle(0)
    , m_paletteHandle(0)
    , m_changeStyleCommand(0)
    , m_xsheetHandle(0)
    , m_hasSavePaletteToolbar(hasSaveToolBar)
    , m_hasPageCommand(hasPageCommand)
    , m_isSaveActionEnabled(true)
    , m_lockPaletteAction(0)
    , m_frozen(false)
    , m_freezePaletteToolButton(0)
    , m_lockPaletteToolButton(0)
    , m_styleNameEditor(nullptr)
    , m_app(0) {
  setObjectName("OnePixelMarginFrame");
  setFrameStyle(QFrame::StyledPanel);

  createTabBar();

  // Create pageView
  m_pageViewerScrollArea = new QScrollArea();
  m_pageViewerScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  m_pageViewerScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  m_pageViewerScrollArea->setWidgetResizable(true);

  m_pageViewer =
      new PageViewer(m_pageViewerScrollArea, m_viewType, hasPasteColors);
  m_pageViewerScrollArea->setWidget(m_pageViewer);
  m_pagesBar->setPageViewer(m_pageViewer);

  // Create toolbar. It is an horizontal layout with three internal toolbar.
  DvScrollWidget *toolbarScrollWidget = new DvScrollWidget;
  QFrame *outerToolbarFrame           = new QFrame(this);
  outerToolbarFrame->setObjectName("ToolBarContainer");
  QWidget *toolBarWidget = new QWidget;  // children of this parent name.
  toolbarScrollWidget->setWidget(toolBarWidget);
  toolBarWidget->setSizePolicy(QSizePolicy::MinimumExpanding,
                               QSizePolicy::Fixed);
  toolBarWidget->setFixedHeight(22);

  m_paletteToolBar     = new QToolBar(toolBarWidget);
  m_savePaletteToolBar = new QToolBar(toolBarWidget);

  QWidget *newPageWidget = new QWidget;  // children of this parent name.
  newPageWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
  newPageWidget->setFixedHeight(22);

  m_newPageToolbar = new QToolBar(this);
  m_newPageToolbar->clear();
  m_newPageToolbar->setMovable(false);
  m_newPageToolbar->setIconSize(QSize(23, 20));
  m_newPageToolbar->setLayoutDirection(Qt::RightToLeft);

  if (m_hasPageCommand) {
    QAction *addPage;
    QIcon addPageIcon = createQIcon("newpage");
    addPage = new QAction(addPageIcon, tr("&New Page"), m_newPageToolbar);
    connect(addPage, SIGNAL(triggered()), this, SLOT(addNewPage()));

    m_newPageToolbar->addAction(addPage);
  }

  QHBoxLayout *newPageLayout = new QHBoxLayout(toolBarWidget);
  newPageLayout->setContentsMargins(0, 0, 0, 0);
  newPageLayout->setSpacing(0);
  newPageLayout->addWidget(m_newPageToolbar, 0, Qt::AlignRight);
  newPageWidget->setLayout(newPageLayout);

  createToolBar();

  QHBoxLayout *toolBarLayout = new QHBoxLayout(toolBarWidget);
  toolBarLayout->setContentsMargins(0, 0, 0, 0);
  toolBarLayout->setSpacing(0);
  {
    toolBarLayout->addWidget(m_paletteToolBar, 0, Qt::AlignLeft);
    toolBarLayout->addStretch();
    toolBarLayout->addWidget(m_savePaletteToolBar, 0, Qt::AlignRight);
  }
  toolBarWidget->setLayout(toolBarLayout);

  // This widget is used to set the background color of the tabBar
  // using the styleSheet.
  // It is also used to take 6px on the left before the tabBar
  // and to draw the two lines on the bottom size
  m_tabBarContainer = new TabBarContainter(this);

  QVBoxLayout *mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);
  {
    // To add 6px (on the left) before the TabBar
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setContentsMargins(0, 0, 0, 0);
    // hLayout->setAlignment(Qt::AlignLeft);
    // hLayout->addSpacing(6);
    {
      hLayout->addWidget(m_pagesBar, 1);
      hLayout->addWidget(newPageWidget, 0);
    }
    m_tabBarContainer->setLayout(hLayout);

    // To align this panel with the style Editor
    mainLayout->addWidget(m_tabBarContainer, 0);
    mainLayout->addWidget(m_pageViewerScrollArea, 1);

    QHBoxLayout *bottomLayout = new QHBoxLayout;
    bottomLayout->setContentsMargins(0, 0, 0, 0);
    bottomLayout->addWidget(toolbarScrollWidget, 0);
    outerToolbarFrame->setLayout(bottomLayout);
    mainLayout->addWidget(outerToolbarFrame);
  }
  setLayout(mainLayout);

  connect(m_pagesBar, SIGNAL(currentChanged(int)), this,
          SLOT(setPageView(int)));
  connect(m_pagesBar, SIGNAL(movePage(int, int)), this,
          SLOT(movePage(int, int)));
  connect(m_pageViewer, SIGNAL(changeWindowTitleSignal()), this,
          SLOT(changeWindowTitle()));
  connect(m_pageViewer, SIGNAL(switchToPage(int)), this,
          SLOT(onSwitchToPage(int)));

  changeWindowTitle();

  setAcceptDrops(true);
}

//-----------------------------------------------------------------------------

PaletteViewer::~PaletteViewer() { delete m_changeStyleCommand; }

//-----------------------------------------------------------------------------

void PaletteViewer::setPaletteHandle(TPaletteHandle *paletteHandle) {
  if (m_paletteHandle == paletteHandle) return;

  bool ret                 = true;
  if (m_paletteHandle) ret = ret && disconnect(m_paletteHandle, 0, this, 0);

  m_paletteHandle = paletteHandle;

  if (m_paletteHandle && isVisible()) {
    ret = ret && connect(m_paletteHandle, SIGNAL(paletteSwitched()), this,
                         SLOT(onPaletteSwitched()));
    ret = ret && connect(m_paletteHandle, SIGNAL(paletteChanged()), this,
                         SLOT(onPaletteChanged()));
    ret = ret && connect(m_paletteHandle, SIGNAL(paletteChanged()), this,
                         SLOT(changeWindowTitle()));
    ret = ret && connect(m_paletteHandle, SIGNAL(paletteTitleChanged()), this,
                         SLOT(changeWindowTitle()));
    ret = ret && connect(m_paletteHandle, SIGNAL(colorStyleSwitched()), this,
                         SLOT(onColorStyleSwitched()));
    ret = ret && connect(m_paletteHandle, SIGNAL(colorStyleChanged(bool)), this,
                         SLOT(changeWindowTitle()));
    ret = ret && connect(m_paletteHandle, SIGNAL(paletteDirtyFlagChanged()),
                         this, SLOT(changeWindowTitle()));
  }

  assert(ret);

  if (m_viewType != CLEANUP_PALETTE)
    m_keyFrameButton->setPaletteHandle(m_paletteHandle);
  m_pageViewer->setPaletteHandle(m_paletteHandle);

  setPageView(0);
  updateTabBar();
  updatePaletteToolBar();
}

//-----------------------------------------------------------------------------

void PaletteViewer::setFrameHandle(TFrameHandle *frameHandle) {
  m_frameHandle = frameHandle;
  if (m_viewType != CLEANUP_PALETTE)
    m_keyFrameButton->setFrameHandle(m_frameHandle);
  m_pageViewer->setFrameHandle(m_frameHandle);
}

//-----------------------------------------------------------------------------

void PaletteViewer::setXsheetHandle(TXsheetHandle *xsheetHandle) {
  m_xsheetHandle = xsheetHandle;
  m_pageViewer->setXsheetHandle(m_xsheetHandle);
}

//-----------------------------------------------------------------------------
/*!for clearing level cache after "paste style" command called from style
 * selection
 */
void PaletteViewer::setLevelHandle(TXshLevelHandle *levelHandle) {
  m_levelHandle = levelHandle;
  m_pageViewer->setLevelHandle(m_levelHandle);
}

//-----------------------------------------------------------------------------

TPalette *PaletteViewer::getPalette() {
  if (!m_paletteHandle) return 0;
  return m_paletteHandle->getPalette();
}

//-----------------------------------------------------------------------------

void PaletteViewer::updateView() {
  changeWindowTitle();
  setPageView(0);
  clearStyleSelection();
  updateTabBar();
  updatePaletteToolBar();
  updateSavePaletteToolBar();
  updatePaletteMenu();
}

//-----------------------------------------------------------------------------

void PaletteViewer::save(QSettings &settings, bool forPopupIni) const {
  int visibleParts = m_toolbarVisibleOtherParts;
  int swatchSize   = m_pageViewer->getViewMode();
  if (m_visibleKeysAction->isChecked()) visibleParts |= 0x01;
  //  if (m_visibleNewAction->isChecked()) visibleParts |= 0x02;
  if (m_visibleGizmoAction->isChecked()) visibleParts |= 0x04;
  if (m_visibleNameAction->isChecked()) visibleParts |= 0x08;
  settings.setValue("toolbarVisibleMsk", visibleParts);
  settings.setValue("swatchSize", swatchSize);
}

void PaletteViewer::load(QSettings &settings) {
  int visibleParts                = 3;  // Show keyframes and new style/page;
  PageViewer::ViewMode swatchSize = PageViewer::SmallChips;
  QVariant visibleVar             = settings.value("toolbarVisibleMsk");
  QVariant swatchVar              = settings.value("swatchSize");
  if (visibleVar.canConvert(QVariant::Int)) visibleParts = visibleVar.toInt();
  if (swatchVar.canConvert(QVariant::Int))
    swatchSize = (PageViewer::ViewMode)swatchVar.toInt();

  m_visibleKeysAction->setChecked(visibleParts & 0x01);
//  m_visibleNewAction->setChecked(visibleParts & 0x02);
  m_visibleGizmoAction->setChecked(visibleParts & 0x04);
  m_visibleNameAction->setChecked(visibleParts & 0x08);
  m_toolbarVisibleOtherParts = visibleParts & ~0x0F;  // Reserve
  m_pageViewer->setViewMode(swatchSize);

  applyToolbarPartVisibility(TBVisKeyframe, visibleParts & 0x01);
//  applyToolbarPartVisibility(TBVisNewStylePage, visibleParts & 0x02);
  applyToolbarPartVisibility(TBVisPaletteGizmo, visibleParts & 0x04);
  applyToolbarPartVisibility(TBVisNameEditor, visibleParts & 0x08);
 }
 
//-----------------------------------------------------------------------------

void PaletteViewer::enableSaveAction(bool enable) {
  if (!m_savePaletteToolBar) return;
  QList<QAction *> actions;
  actions               = m_savePaletteToolBar->actions();
  enable                = !getPalette() ? false : enable;
  m_isSaveActionEnabled = enable;
  int i;
  for (i = 0; i < actions.count() - 1; i++) {
    QAction *act = actions[i];
    if (act->text() == tr("&Save Palette As") ||
        act->text() == tr("&Save Palette") ||
        act->text() == tr("&Save As Default Vector Palette") ||
        act->text() == tr("&Save As Default Smart Raster Palette") ||
        act->text() == tr("&Save As Default Raster Palette") ||
        act->text() == tr("&Save As Default Palette"))
      act->setEnabled(enable);
  }
}

//-----------------------------------------------------------------------------

void PaletteViewer::applyToolbarPartVisibility(int part, bool visible) {
  assert(m_toolbarParts.contains(part));
  for (QAction *action : m_toolbarParts.values(part)) {
    action->setVisible(visible);
  }
}

//-----------------------------------------------------------------------------

void PaletteViewer::toggleKeyframeVisibility(bool checked) {
  applyToolbarPartVisibility(TBVisKeyframe, checked);
}

//void PaletteViewer::toggleNewStylePageVisibility(bool checked) {
//  applyToolbarPartVisibility(TBVisNewStylePage, checked);
//}

void PaletteViewer::togglePaletteGizmoVisibility(bool checked) {
  applyToolbarPartVisibility(TBVisPaletteGizmo, checked);
}

void PaletteViewer::toggleNameEditorVisibility(bool checked) {
  applyToolbarPartVisibility(TBVisNameEditor, checked);
}

//-----------------------------------------------------------------------------
/*! Create tab bar to select palette page.
 */
void PaletteViewer::createTabBar() {
  m_pagesBar = new PaletteTabBar(this, m_hasPageCommand);

  connect(m_pagesBar, SIGNAL(tabTextChanged(int)), this,
          SLOT(onTabTextChanged(int)));

  if (!getPalette()) return;

  updateTabBar();
}

//-----------------------------------------------------------------------------
/*! Create right part of button bar.
 */
void PaletteViewer::createPaletteToolBar() {
  m_paletteToolBar->clear();
  m_paletteToolBar->setMovable(false);
  m_paletteToolBar->setIconSize(QSize(20, 20));
  m_paletteToolBar->setLayoutDirection(Qt::LeftToRight);

  m_toolbarParts.clear();

  // m_toolbarParts.insert(TBVisNewStylePage, addPage);
  // QIcon newColorIcon = createQIcon("newstyle");
  // QAction* addColor =
  //    new QAction(newColorIcon, tr("&New Style"), m_paletteToolBar);
  // connect(addColor, SIGNAL(triggered()), this, SLOT(addNewColor()));

  // m_paletteToolBar->addAction(addColor);

  // m_paletteToolBar->addSeparator();
  // m_toolbarParts.insert(TBVisNewStylePage, addColor);
  // m_toolbarParts.insert(TBVisNewStylePage, m_paletteToolBar->addSeparator());

  // KeyFrame button
  if (m_viewType != CLEANUP_PALETTE) {
    m_keyFrameButton = new PaletteKeyframeNavigator(m_paletteToolBar);
    m_toolbarParts.insert(TBVisKeyframe, m_paletteToolBar->addWidget(m_keyFrameButton));
    m_toolbarParts.insert(TBVisKeyframe, m_paletteToolBar->addSeparator());
    m_keyFrameButton->setSelection(m_pageViewer->getSelection());
  }

  // Lock button to avoid editing the palette by mistake
  if (m_viewType == LEVEL_PALETTE) {
    m_lockPaletteToolButton = new QToolButton(this);
    m_lockPaletteToolButton->setIcon(createQIcon("lock"));
    m_lockPaletteToolButton->setCheckable(true);
    m_lockPaletteToolButton->setObjectName("PaletteLockButton");
    m_lockPaletteToolButton->setToolTip(tr("Lock Palette"));
    if (getPalette()) {
      m_lockPaletteToolButton->setChecked(getPalette()->isLocked());
    }

    connect(m_lockPaletteToolButton, SIGNAL(clicked(bool)), this,
            SLOT(setIsLocked(bool)));
    m_paletteToolBar->addWidget(m_lockPaletteToolButton);

    m_freezePaletteToolButton = new QToolButton(this);
    m_freezePaletteToolButton->setIcon(createQIcon("pane_freeze"));
    m_freezePaletteToolButton->setCheckable(true);
    m_freezePaletteToolButton->setObjectName("PaletteLockButton");
    m_freezePaletteToolButton->setToolTip(tr("Stay on Current Palette"));

    connect(m_freezePaletteToolButton, &QToolButton::clicked,
            [=](bool checked) { emit frozenChanged(checked); });
    m_paletteToolBar->setStyleSheet("QToolBar{spacing:3px;}");
    m_paletteToolBar->addWidget(m_freezePaletteToolButton);

    m_paletteToolBar->addSeparator();
    CommandManager *cmd = CommandManager::instance();
    m_sharedGizmoAction = cmd->getAction("MI_OpenPltGizmo");

    // Clone palette gizmo action so visibility can be control
    QAction *palGizmo = new DVAction(m_sharedGizmoAction->icon(),
                                     m_sharedGizmoAction->text(), this);
    connect(palGizmo, &QAction::triggered,
            [&]() { m_sharedGizmoAction->trigger(); });
    m_paletteToolBar->addAction(palGizmo);
    m_toolbarParts.insert(TBVisPaletteGizmo, palGizmo);
  } else if (m_viewType == STUDIO_PALETTE) {
    QToolButton *toolButton = new QToolButton(this);
    toolButton->setPopupMode(QToolButton::InstantPopup);
    toolButton->setIcon(createQIcon("lock"));
    toolButton->setObjectName("PaletteLockButton");
    toolButton->setToolTip(tr("Lock Palette"));
    toolButton->setCheckable(true);

    QMenu *lockMenu     = new QMenu(toolButton);
    m_lockPaletteAction = new QAction(tr("&Lock Palette"), toolButton);
    m_lockPaletteAction->setCheckable(true);
    lockMenu->addAction(m_lockPaletteAction);
    toolButton->setMenu(lockMenu);
    if (getPalette()) {
      m_lockPaletteAction->setChecked(getPalette()->isLocked());
    }

    connect(m_lockPaletteAction, SIGNAL(triggered(bool)), this,
            SLOT(setIsLocked(bool)));
    connect(m_lockPaletteAction, SIGNAL(toggled(bool)), toolButton,
            SLOT(setChecked(bool)));

    m_paletteToolBar->addWidget(toolButton);
    m_paletteToolBar->addSeparator();
  }

  QAction *openStyleNameEditorAct = new QAction(tr("Name Editor"));
  openStyleNameEditorAct->setIcon(createQIcon("rename", false, true));
  connect(openStyleNameEditorAct, &QAction::triggered, [&]() {
    if (!m_styleNameEditor) {
      m_styleNameEditor = new StyleNameEditor(this);
      m_styleNameEditor->setPaletteHandle(getPaletteHandle());
    }
    m_styleNameEditor->show();
    m_styleNameEditor->raise();
    m_styleNameEditor->activateWindow();
  });
  m_paletteToolBar->addAction(openStyleNameEditorAct);
  m_toolbarParts.insert(TBVisNameEditor, openStyleNameEditorAct);

  updatePaletteToolBar();
}

void PaletteViewer::setIsFrozen(bool frozen) {
  m_frozen = frozen;
  m_freezePaletteToolButton->setChecked(frozen);
}

//-----------------------------------------------------------------------------
/*! Create left part of button bar; insert different actions in according to
current viewer palette type.
*/
void PaletteViewer::createSavePaletteToolBar() {
  m_savePaletteToolBar->clear();
  m_savePaletteToolBar->setMovable(false);
  m_savePaletteToolBar->setIconSize(QSize(22, 20));

  QWidget *spacer = new QWidget();
  spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
  // toolBar is a pointer to an existing toolbar
  m_savePaletteToolBar->addWidget(spacer);

  if (!m_hasSavePaletteToolbar || m_viewType == CLEANUP_PALETTE) {
    m_savePaletteToolBar->hide();
    return;
  }

  if (m_viewType == LEVEL_PALETTE) {
    // save load palette
    PaletteIconWidget *movePalette =
        new PaletteIconWidget(m_savePaletteToolBar);
    movePalette->setToolTip(
        tr("Drag this icon to a Studio or Project palette to add it."));
    connect(movePalette, SIGNAL(startDrag()), this, SLOT(startDragDrop()));

    QAction *act = m_savePaletteToolBar->addWidget(movePalette);
    act->setText(tr("&Move Palette"));
  }

  // Attenzione: alcune modifiche sono state fatte a livello di stylesheet
  QToolButton *viewModeButton = new QToolButton(this);
  viewModeButton->setPopupMode(QToolButton::InstantPopup);
  viewModeButton->setToolTip(tr("Options"));

  QIcon viewModeIcon = createQIcon("menu");
  viewModeButton->setIcon(viewModeIcon);
  m_viewMode = new QMenu(QString("Options"), viewModeButton);
  m_viewMode->setLayoutDirection(Qt::LeftToRight);

  QActionGroup *viewModeGroup = new QActionGroup(m_viewMode);
  viewModeGroup->setExclusive(true);
  connect(viewModeGroup, SIGNAL(triggered(QAction *)), this,
          SLOT(onViewMode(QAction *)));

  auto addViewAction = [&](const QString &label, PageViewer::ViewMode mode) {
    QAction *viewAction = new QAction(label, m_viewMode);
    viewAction->setData(mode);
    viewAction->setCheckable(true);
    if (m_pageViewer->getViewMode() == mode) viewAction->setChecked(true);
    viewModeGroup->addAction(viewAction);
    m_viewMode->addAction(viewAction);
  };

  addViewAction(tr("&Small Thumbnails View"), PageViewer::SmallChips);
  addViewAction(tr("&Medium Thumbnails View"), PageViewer::MediumChips);
  addViewAction(tr("&Large Thumbnails View"), PageViewer::LargeChips);
  addViewAction(tr("&List View"), PageViewer::List);

  m_viewMode->addSeparator();
  m_showStyleIndex = new QAction(tr("Show Style Index"), this);
  m_showStyleIndex->setCheckable(true);
  if (m_pageViewer)
    m_showStyleIndex->setChecked(m_pageViewer->getShowStyleIndex());
  connect(m_showStyleIndex, &QAction::toggled, [=](bool checked) {
    if (m_pageViewer) m_pageViewer->toggleShowStyleIndex();
    if (m_pageViewer) m_pageViewer->update();
    bool setChecked              = false;
    if (m_pageViewer) setChecked = m_pageViewer->getShowStyleIndex();
    m_showStyleIndex->setChecked(setChecked);
  });

  connect(m_viewMode, &QMenu::aboutToShow, [=]() {
    m_showStyleIndex->blockSignals(true);
    m_showStyleIndex->setChecked(m_pageViewer->getShowStyleIndex());
    m_showStyleIndex->blockSignals(false);
  });
  m_viewMode->addAction(m_showStyleIndex);

  m_viewMode->addSeparator();

  // Add ability to show or hide buttons
  QMenu *visibleButtons = new QMenu(tr("Visible Toolbar Buttons"));

  m_visibleKeysAction = new QAction(tr("KeyFrame"));
  m_visibleKeysAction->setCheckable(true);
  m_visibleKeysAction->setChecked(true);
  visibleButtons->addAction(m_visibleKeysAction);
//  m_visibleNewAction = new QAction(tr("New Style/Page"));
//  m_visibleNewAction->setCheckable(true);
//  m_visibleNewAction->setChecked(true);
//  visibleButtons->addAction(m_visibleNewAction);
  m_visibleGizmoAction = new QAction(tr("Palette Gizmo"));
  m_visibleGizmoAction->setCheckable(true);
  m_visibleGizmoAction->setChecked(true);
  visibleButtons->addAction(m_visibleGizmoAction);
  m_visibleNameAction = new QAction(tr("Name Editor"));
  m_visibleNameAction->setCheckable(true);
  m_visibleNameAction->setChecked(true);
  visibleButtons->addAction(m_visibleNameAction);
  m_viewMode->addMenu(visibleButtons);

  if (m_viewType == CLEANUP_PALETTE) m_visibleKeysAction->setVisible(false);

  if (m_viewType != LEVEL_PALETTE) m_visibleGizmoAction->setVisible(false);

  connect(m_visibleKeysAction, SIGNAL(toggled(bool)), this,
          SLOT(toggleKeyframeVisibility(bool)));
//  connect(m_visibleNewAction, SIGNAL(toggled(bool)), this,
//          SLOT(toggleNewStylePageVisibility(bool)));
  connect(m_visibleGizmoAction, SIGNAL(toggled(bool)), this,
          SLOT(togglePaletteGizmoVisibility(bool)));
  connect(m_visibleNameAction, SIGNAL(toggled(bool)), this,
          SLOT(toggleNameEditorVisibility(bool)));

  if (m_viewType == STUDIO_PALETTE) {
    QIcon savePaletteIcon = createQIcon("save", false, true);
    QAction *savePalette =
        new QAction(savePaletteIcon, tr("&Save Palette"), m_savePaletteToolBar);
    savePalette->setToolTip(tr("Save the palette."));

    connect(savePalette, SIGNAL(triggered()), this, SLOT(saveStudioPalette()));
    // m_viewMode->addSeparator();
    // m_viewMode->addAction(savePalette);
    m_savePaletteToolBar->addAction(savePalette);
  } else if (m_viewType == LEVEL_PALETTE) {
    // save load palette
    m_viewMode->addSeparator();

    // overwrite palette
    QAction *savePalette =
        CommandManager::instance()->getAction("MI_OverwritePalette");
    m_viewMode->addAction(savePalette);

    // save palette as
    QAction *saveAsPalette =
        CommandManager::instance()->getAction("MI_SavePaletteAs");
    m_viewMode->addAction(saveAsPalette);

    // save as default palette
    QAction *saveDefaultPalette =
        CommandManager::instance()->getAction("MI_SaveAsDefaultPalette");
    m_viewMode->addAction(saveDefaultPalette);
  }

  viewModeButton->setMenu(m_viewMode);

  m_savePaletteToolBar->addWidget(viewModeButton);

  updateSavePaletteToolBar();
  updatePaletteMenu();
}

//-----------------------------------------------------------------------------
/*! Update page tab bar adding or removing tab in accord with viewer palette.
 */
void PaletteViewer::updateTabBar() {
  int tabCount = m_pagesBar->count();
  int i;

  // Se ci sono tab li butto
  for (i = tabCount - 1; i >= 0; i--) m_pagesBar->removeTab(i);

  TPalette *palette = getPalette();
  if (!palette) return;

  QIcon tabIcon = createQIcon("palette_tab");
  m_pagesBar->setIconSize(QSize(16, 16));

  // Aggiungo i tab in funzione delle pagine di m_palette
  for (i = 0; i < palette->getPageCount(); i++) {
    TPalette::Page *page = palette->getPage(i);
    std::wstring ws      = page->getName();
    QString pageName     = QString::fromStdWString(ws);
    m_pagesBar->addTab(tabIcon, pageName);
  }
  m_pagesBar->update();
}

//-----------------------------------------------------------------------------
/*! Update right button bar, enable its action if current viewer palette is
 * empty.
 */
void PaletteViewer::updatePaletteToolBar() {
  if (!m_paletteToolBar) return;
  QList<QAction *> actions;
  actions                = m_paletteToolBar->actions();
  TPalette *palette      = getPalette();
  bool enable            = !palette ? false : true;
  bool enableNewStyleAct = enable;
  // limit the number of cleanup styles to 7
  if (palette && palette->isCleanupPalette())
    enableNewStyleAct = (palette->getStyleInPagesCount() < 8);

  if (m_viewType != CLEANUP_PALETTE) m_keyFrameButton->setEnabled(enable);
  int i;
  for (i = 0; i < actions.count(); i++) {
    QAction *act = actions[i];
    if (act->text() == tr("&New Style")) {
      act->setEnabled(enableNewStyleAct);
      continue;
    }
    act->setEnabled(enable);
  }
  actions.clear();
  actions = m_newPageToolbar->actions();
  for (i = 0; i < actions.count(); i++) {
    QAction *act = actions[i];
    if (act->text() == tr("&New Style")) {
      act->setEnabled(enableNewStyleAct);
      continue;
    }
    act->setEnabled(enable);
  }
}

//-----------------------------------------------------------------------------
/*! Update left button bar, enable its action if current viewer palette is
 * empty.
 */
void PaletteViewer::updateSavePaletteToolBar() {
  if (!m_savePaletteToolBar) return;
  QList<QAction *> actions;
  actions     = m_savePaletteToolBar->actions();
  bool enable = !getPalette() ? false : true;

  int i;
  for (i = 0; i < actions.count(); i++) {
    QAction *act = actions[i];

    if (act->text() == tr("&Save Palette As") ||
        act->text() == tr("&Save Palette") ||
        act->text() == tr("&Palette Gizmo"))
      act->setEnabled(enable);
    else if (act->text() == tr("&Save As Default Vector Palette") ||
             act->text() == tr("&Save As Default Smart Raster Palette") ||
             act->text() == tr("&Save As Default Raster Palette") ||
             act->text() == tr("&Save As Default Palette")) {
      if (m_levelHandle) {
        int levelType = m_levelHandle->getLevel()
                            ? m_levelHandle->getLevel()->getType()
                            : getPalette()
                                  ? getPalette()->getDefaultPaletteType()
                                  : UNKNOWN_XSHLEVEL;
        if (levelType == UNKNOWN_XSHLEVEL)
          levelType = Preferences::instance()->getDefLevelType();
        setSaveDefaultText(act, levelType);
        if (levelType == PLT_XSHLEVEL) enable = false;
      }
      act->setEnabled(enable);
    } else if (m_viewType != STUDIO_PALETTE && i == 1) {  // move action
      // Do not hide when palette is not visible otherwise it causes
      // panel redistribution at startup
      bool moveVisible = isVisible() ? enable : true;
      actions[i]->setVisible(moveVisible);
    } else
      actions[i]->setEnabled(true);
  }
}

//-----------------------------------------------------------------------------

void PaletteViewer::updatePaletteMenu() {
  if (!m_viewMode) return;
  QList<QAction *> actions;
  actions                   = m_viewMode->actions();
  bool enable               = !getPalette() ? false : true;
  PageViewer::ViewMode mode = m_pageViewer->getViewMode();
  int i;
  for (i = 0; i < actions.count(); i++) {
    QAction *act = actions[i];

    if (act->text() == tr("&Save Palette As") ||
        act->text() == tr("&Save Palette") ||
        act->text() == tr("&Palette Gizmo"))
      act->setEnabled(enable);
    else if (act->text() == tr("&Save As Default Vector Palette") ||
             act->text() == tr("&Save As Default Smart Raster Palette") ||
             act->text() == tr("&Save As Default Raster Palette") ||
             act->text() == tr("&Save As Default Palette")) {
      if (m_levelHandle) {
        int levelType = m_levelHandle->getLevel()
                            ? m_levelHandle->getLevel()->getType()
                            : getPalette()
                                  ? getPalette()->getDefaultPaletteType()
                                  : UNKNOWN_XSHLEVEL;
        if (levelType == UNKNOWN_XSHLEVEL)
          levelType = Preferences::instance()->getDefLevelType();
        setSaveDefaultText(act, levelType);
        if (levelType == PLT_XSHLEVEL) enable = false;
      }
      act->setEnabled(enable);
    } else
      actions[i]->setEnabled(true);

    if (act->data().canConvert<int>()) {
      int i = act->data().toInt();
      if (i == static_cast<int>(mode)) {
        act->setChecked(true);
      }
    }
  }
}

//-----------------------------------------------------------------------------
/*! Resize the widget and its child.
 */
void PaletteViewer::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  if (m_pageViewer) m_pageViewer->computeSize();
  if (m_pagesBar) m_pagesBar->setMaximumWidth(width() - 12);
}

//-----------------------------------------------------------------------------

void PaletteViewer::setChangeStyleCommand(
    ChangeStyleCommand *changeStyleCommand) {
  if (m_changeStyleCommand) delete m_changeStyleCommand;
  m_changeStyleCommand = changeStyleCommand;
  m_pageViewer->setChangeStyleCommand(m_changeStyleCommand);
}

//-----------------------------------------------------------------------------
/*! Create and open the Right-click menu.
 */
void PaletteViewer::setSaveDefaultText(QAction *action, int levelType) {
  switch (levelType) {
  case PLI_XSHLEVEL:
    action->setText(tr("&Save As Default Vector Palette"));
    break;
  case TZP_XSHLEVEL:
    action->setText(tr("&Save As Default Smart Raster Palette"));
    break;
  case OVL_XSHLEVEL:
    action->setText(tr("&Save As Default Raster Palette"));
    break;
  default:
    action->setText(tr("&Save As Default Palette"));
    break;
  }
}

void PaletteViewer::contextMenuEvent(QContextMenuEvent *event) {
  m_indexPageToDelete = -1;
  QPoint pos          = event->pos();
  if (!getPalette() || !m_tabBarContainer->geometry().contains(pos)) return;

  QMenu *menu = new QMenu(this);
  if (m_hasPageCommand) {
    QAction *newPage =
        menu->addAction(createQIcon("newpage", false, true), tr("New Page"));
    connect(newPage, SIGNAL(triggered()), SLOT(addNewPage()));

    if (m_pagesBar->geometry().contains(pos)) {
      int tabIndex         = m_pagesBar->tabAt(pos);
      TPalette::Page *page = getPalette()->getPage(tabIndex);
      if (page) {
        bool canRemovePage = true;
        if (page->getStyleId(0) == 0 || page->getStyleId(1) == 1)
          canRemovePage = false;
        if (canRemovePage) {
          m_indexPageToDelete = tabIndex;
          QAction *deletePage = menu->addAction(
              createQIcon("delete", false, true), tr("Delete Page"));
          connect(deletePage, SIGNAL(triggered()), SLOT(deletePage()));
        }
      }
    }
  }

  if (m_viewType == LEVEL_PALETTE && m_isSaveActionEnabled) {
    menu->addSeparator();
    menu->addAction(
        CommandManager::instance()->getAction("MI_OverwritePalette"));
    menu->addAction(CommandManager::instance()->getAction("MI_SavePaletteAs"));
    QAction *action =
        CommandManager::instance()->getAction("MI_SaveAsDefaultPalette");
    menu->addAction(action);
    bool enable = true;
    if (m_levelHandle) {
      int levelType = m_levelHandle->getLevel()
                          ? m_levelHandle->getLevel()->getType()
                          : getPalette() ? getPalette()->getDefaultPaletteType()
                                         : UNKNOWN_XSHLEVEL;
      if (levelType == UNKNOWN_XSHLEVEL)
        levelType = Preferences::instance()->getDefLevelType();
      setSaveDefaultText(action, levelType);
      if (levelType == PLT_XSHLEVEL) enable = false;
    }
    action->setEnabled(enable);
  }

  if (m_viewType == LEVEL_PALETTE && !getPalette()->isLocked() &&
      m_isSaveActionEnabled &&
      !FullColorPalette::instance()->isFullColorPalette(getPalette())) {
    menu->addSeparator();
    menu->addAction(
        CommandManager::instance()->getAction("MI_EraseUnusedStyles"));
  }

  menu->exec(event->globalPos());
}

//-----------------------------------------------------------------------------

void PaletteViewer::mousePressEvent(QMouseEvent *event) {
  QFrame::mousePressEvent(event);
  if (event->button() == Qt::RightButton) {
    m_pageViewer->getSelection()->makeCurrent();
    m_pageViewer->updateCommandLocks();
  }
}

//-----------------------------------------------------------------------------

void PaletteViewer::showEvent(QShowEvent *) {
  onPaletteSwitched();
  changeWindowTitle();

  if (!m_paletteHandle) return;

  connect(m_paletteHandle, SIGNAL(paletteSwitched()), this,
          SLOT(onPaletteSwitched()));
  connect(m_paletteHandle, SIGNAL(paletteChanged()), this,
          SLOT(onPaletteChanged()));
  connect(m_paletteHandle, SIGNAL(paletteTitleChanged()), this,
          SLOT(changeWindowTitle()));
  connect(m_paletteHandle, SIGNAL(colorStyleSwitched()), this,
          SLOT(onColorStyleSwitched()));
  connect(m_paletteHandle, SIGNAL(colorStyleChanged(bool)), this,
          SLOT(changeWindowTitle()));
  connect(m_paletteHandle, SIGNAL(paletteDirtyFlagChanged()), this,
          SLOT(changeWindowTitle()));
  if (m_viewType == STUDIO_PALETTE) {
    CommandManager::instance()
        ->getAction("MI_SaveStudioPalette")
        ->setEnabled(true);
    connect(CommandManager::instance()->getAction("MI_SaveStudioPalette"),
            SIGNAL(triggered()), this, SLOT(saveStudioPalette()));
  }

  if (!m_frameHandle) return;
  // Connessione necessaria per aggiornare lo stile in caso di palette animate.
  connect(m_frameHandle, SIGNAL(frameSwitched()), this,
          SLOT(onFrameSwitched()));
}

//-----------------------------------------------------------------------------

void PaletteViewer::hideEvent(QHideEvent *) {
  disconnect(m_paletteHandle, SIGNAL(paletteSwitched()), this,
             SLOT(onPaletteSwitched()));
  disconnect(m_paletteHandle, SIGNAL(paletteChanged()), this,
             SLOT(onPaletteChanged()));
  disconnect(m_paletteHandle, SIGNAL(paletteTitleChanged()), this,
             SLOT(changeWindowTitle()));
  disconnect(m_paletteHandle, SIGNAL(colorStyleSwitched()), this,
             SLOT(onColorStyleSwitched()));
  disconnect(m_paletteHandle, SIGNAL(colorStyleChanged(bool)), this,
             SLOT(changeWindowTitle()));
  disconnect(m_paletteHandle, SIGNAL(paletteDirtyFlagChanged()), this,
             SLOT(changeWindowTitle()));
  disconnect(m_paletteHandle,
             SIGNAL(broadcastColorStyleChangedOnMouseRelease()), this,
             SLOT(onPaletteChanged()));
  if (m_viewType == STUDIO_PALETTE) {
    CommandManager::instance()
        ->getAction("MI_SaveStudioPalette")
        ->setEnabled(false);
    disconnect(CommandManager::instance()->getAction("MI_SaveStudioPalette"),
               SIGNAL(triggered()), this, SLOT(saveStudioPalette()));
  }

  if (!m_frameHandle) return;
  disconnect(m_frameHandle, SIGNAL(frameSwitched()), this,
             SLOT(onFrameSwitched()));
}

//-----------------------------------------------------------------------------

void PaletteViewer::enterEvent(QEvent *) {
  if (m_app) m_app->getPaletteController()->setCurrentPaletteViewer(this);
}

//-----------------------------------------------------------------------------
/*! If currente palette viewer exist verify event data, if is a PaletteData or
has urls accept event.
*/
void PaletteViewer::dragEnterEvent(QDragEnterEvent *event) {
  TPalette *palette = getPalette();
  if (!palette || m_viewType == CLEANUP_PALETTE) return;

  const QMimeData *mimeData      = event->mimeData();
  const PaletteData *paletteData = dynamic_cast<const PaletteData *>(mimeData);

  if (paletteData) {
    // Sto "draggando" stili.
    if (paletteData->hasStyleIndeces()) {
      m_pageViewer->createDropPage();
      if (!palette) onSwitchToPage(palette->getPageCount() - 1);
    }
    // Accetto l'evento
    event->acceptProposedAction();
    return;
  }

  if (!acceptResourceDrop(mimeData->urls())) return;
  QList<QUrl> urls = mimeData->urls();
  int count        = urls.size();
  if (count == 0) return;
  // Accetto l'evento solo se ho tutte palette.
  int i;
  for (i = 0; i < count; i++) {
    TFilePath path(urls[i].toLocalFile().toStdWString());
    if (!path.getType().empty() && path.getType() != "tpl") return;
  }

  // Force CopyAction
  event->setDropAction(Qt::CopyAction);
  // For files, don't accept original proposed action in case it's a move
  event->accept();
}

//-----------------------------------------------------------------------------
/*! Execute drop event.
 */
void PaletteViewer::dropEvent(QDropEvent *event) {
  if (m_viewType == CLEANUP_PALETTE) return;
  const QMimeData *mimeData = event->mimeData();

  QPoint tollBarPos      = m_savePaletteToolBar->mapFrom(this, event->pos());
  QAction *currentAction = m_savePaletteToolBar->actionAt(tollBarPos);
  bool loadPalette =
      currentAction && currentAction->text() == QString(tr("&Move Palette"));

  // ho i path delle palette
  if (mimeData->hasUrls()) {
    QList<QUrl> urls = mimeData->urls();
    int count        = urls.size();
    if (count == 0) return;

    int i;
    for (i = 0; i < count; i++) {
      TFilePath path(urls[i].toLocalFile().toStdWString());
      if (!path.getType().empty() && path.getType() != "tpl") return;
      if (loadPalette && i == 0) {
        if (m_xsheetHandle) {
          TPalette *newPalette =
              StudioPalette::instance()->getPalette(path, false);
          if (!newPalette) return;
          int ret = DVGui::eraseStylesInDemand(getPalette(), m_xsheetHandle,
                                               newPalette);
          if (ret == 0) return;
        }
        StudioPaletteCmd::loadIntoCurrentPalette(m_paletteHandle, path);
      } else {
        int nextPageIndex = m_paletteHandle->getPalette()->getPageCount();
        StudioPaletteCmd::mergeIntoCurrentPalette(m_paletteHandle, path);
        if (!i) onSwitchToPage(nextPageIndex);
      }

      if (loadPalette) {
        TFilePath refImagePath =
            StudioPalette::instance()->getPalette(path, true)->getRefImgPath();
        if (!refImagePath.isEmpty() &&
            getPalette()->getRefImgPath().isEmpty()) {
          getPalette()->setRefImgPath(refImagePath);
          m_paletteHandle->notifyPaletteChanged();
        }
      }
    }
    // Force CopyAction
    event->setDropAction(Qt::CopyAction);
    // For files, don't accept original proposed action in case it's a move
    event->accept();
    return;
  }

  const PaletteData *paletteData = dynamic_cast<const PaletteData *>(mimeData);

  if (!paletteData) return;
  // Sto inserendo stili
  if (paletteData->hasStyleIndeces()) {
    m_pageViewer->drop(-1, mimeData);
    event->acceptProposedAction();
  } else {
    // Ho la palette da inserire
    TPalette *palette = paletteData->getPalette();
    if (getPalette() == palette) return;
    if (loadPalette) {
      if (m_xsheetHandle) {
        int ret =
            DVGui::eraseStylesInDemand(getPalette(), m_xsheetHandle, palette);
        if (ret == 0) return;
      }
      StudioPaletteCmd::loadIntoCurrentPalette(m_paletteHandle, palette);
    } else {
      int nextPageIndex = m_paletteHandle->getPalette()->getPageCount();
      StudioPaletteCmd::mergeIntoCurrentPalette(m_paletteHandle, palette);
      onSwitchToPage(nextPageIndex);
    }
  }
}

//-----------------------------------------------------------------------------
/*! Start drag and drop; if current page exist set drag and drop event data.
 */
void PaletteViewer::startDragDrop() {
  TRepetitionGuard guard;
  if (!guard.hasLock()) return;

  if (m_viewType == CLEANUP_PALETTE) return;

  assert(m_viewType != STUDIO_PALETTE && m_viewType != CLEANUP_PALETTE);

  TPalette *palette = getPalette();
  if (!palette) return;

  QDrag *drag              = new QDrag(this);
  PaletteData *paletteData = new PaletteData();
  paletteData->setPalette(palette);
  drag->setMimeData(paletteData);

  Qt::DropAction dropAction = drag->exec(Qt::CopyAction | Qt::MoveAction);
}

//-----------------------------------------------------------------------------

void PaletteViewer::clearStyleSelection() { m_pageViewer->clearSelection(); }

//-----------------------------------------------------------------------------
/*! Set current view page to \b currentIndexPage
 */
void PaletteViewer::setPageView(int currentIndexPage) {
  TPalette *palette    = getPalette();
  TPalette::Page *page = palette ? palette->getPage(currentIndexPage) : 0;
  m_currentIndexPage   = palette ? currentIndexPage : 0;
  m_pageViewer->setPage(page);
}

//-----------------------------------------------------------------------------
/*! If current palette viewer is not empty create emit a signal to create new
palette page.
*/
void PaletteViewer::addNewPage() {
  TPalette *palette = getPalette();
  if (palette) {
    if (palette->isLocked()) return;

    updateTabBar();
    PaletteCmd::addPage(m_paletteHandle);
    onSwitchToPage(m_paletteHandle->getPalette()->getPageCount() - 1);
  }
}

//-----------------------------------------------------------------------------
/*! Create a new style in current page view of current palette viewer emit a
signal
to create a new style.
*/
void PaletteViewer::addNewColor() {
  if (!getPalette() || getPalette()->isLocked()) return;

  TPalette::Page *page = m_pageViewer->getPage();
  update();
  PaletteCmd::createStyle(m_paletteHandle, page);
  m_pageViewer->computeSize();
  if (m_viewType == CLEANUP_PALETTE) updatePaletteToolBar();
}

//-----------------------------------------------------------------------------
/*! Emit a signal to delete a page of current palette viewer.
 */
void PaletteViewer::deletePage() {
  TPalette *palette = getPalette();
  if (!palette || palette->isLocked()) return;

  if (m_xsheetHandle) {
    std::vector<int> styleIds;
    TPalette::Page *page = palette->getPage(m_indexPageToDelete);
    if (!page) return;  // La pagina dovrebbe esserci sempre
    int i;
    for (i = 0; i < page->getStyleCount(); i++)
      styleIds.push_back(page->getStyleId(i));

    int ret = DVGui::eraseStylesInDemand(palette, styleIds, m_xsheetHandle);
    if (ret == 0) return;
  }

  PaletteCmd::destroyPage(m_paletteHandle, m_indexPageToDelete);
  updateTabBar();

  palette->setDirtyFlag(true);
  if (m_viewType == CLEANUP_PALETTE) updatePaletteToolBar();
}

//-----------------------------------------------------------------------------
/*! If current palette view is studio palette and palette has a global name
save current viewer palette in studio palette.
*/
void PaletteViewer::saveStudioPalette() {
  StudioPalette *sp = StudioPalette::instance();
  TPalette *palette = getPalette();
  if (!palette) {
    DVGui::warning("No current palette");
    return;
  }
  std::wstring gname = palette->getGlobalName();
  if (gname.empty()) {
    StudioPaletteViewer *parentSPV =
        qobject_cast<StudioPaletteViewer *>(parentWidget());
    if (!parentSPV) {
      DVGui::warning("No GlobalName");
      return;
    } else {
      TFilePath palettePath = parentSPV->getCurrentItemPath();
      if (palettePath.isEmpty())
        DVGui::warning("No GlobalName, No Filepath");
      else {
        QString question;
        question = "Do you want to overwrite current palette to " +
                   toQString(palettePath) + " ?";
        int ret = DVGui::MsgBox(question, QObject::tr("Overwrite"),
                                QObject::tr("Don't Overwrite"), 0);
        if (ret == 2 || ret == 0) return;
        try {
          StudioPalette::instance()->save(palettePath, palette);
          palette->setDirtyFlag(false);
        } catch (TSystemException se) {
          QApplication::restoreOverrideCursor();
          DVGui::warning(QString::fromStdWString(se.getMessage()));
          return;
        } catch (...) {
        }
      }
    }
    return;
  }

  TFilePath fp = sp->getPalettePath(gname);
  if (fp != TFilePath()) {
    QString question;
    question = "Do you want to overwrite current studio palette to " +
               toQString(fp) + " ?";
    int ret =
        DVGui::MsgBox(question, tr("Overwrite"), tr("Don't Overwrite"), 0);
    if (ret == 2 || ret == 0) return;
    try {
      sp->setPalette(fp, getPalette(), false);
    } catch (TSystemException se) {
      DVGui::warning(QString::fromStdWString(se.getMessage()));
      return;
    } catch (...) {
      DVGui::warning(QString::fromStdWString(fp.getWideString() + L"\n") +
                     tr("Failed to save palette."));
      return;
    }

    StudioPaletteCmd::updateAllLinkedStyles(m_paletteHandle, m_xsheetHandle);

    palette->setDirtyFlag(false);
  }

  m_paletteHandle->notifyPaletteChanged();
}

//-----------------------------------------------------------------------------
/*! If current color switched update current page view.
 */
void PaletteViewer::onColorStyleSwitched() {
  TPalette *palette = getPalette();

  // se non c'e' palette, pageviewer p pagina corrente esco (non dovrebbe
  // succedere mai)
  if (!palette || !m_pageViewer) return;
  int styleIndex = m_paletteHandle->getStyleIndex();

  setPageView(m_pagesBar->currentIndex());

  TPalette::Page *page = m_pageViewer->getPage();
  if (!page) return;

  // faccio in modo che la pagina che contiene il colore selezionato diventi
  // corrente
  int indexInPage = page->search(styleIndex);
  if (indexInPage == -1) {
    if (!palette->getStylePage(styleIndex)) return;
    int pageIndex = palette->getStylePage(styleIndex)->getIndex();
    onSwitchToPage(pageIndex);
    indexInPage = m_pageViewer->getPage()->search(styleIndex);
  }

  QRect colorStyleRect = m_pageViewer->getItemRect(indexInPage);
  m_pageViewerScrollArea->ensureVisible(colorStyleRect.center().x(),
                                        colorStyleRect.center().y(),
                                        colorStyleRect.size().width() / 2 + 4,
                                        colorStyleRect.size().height() / 2 + 4);
  m_pageViewer->update();
}

//-----------------------------------------------------------------------------
/*! Update view. Remember current page bar index.
 */
void PaletteViewer::onPaletteChanged() {
  int index = m_pagesBar->currentIndex();
  updateTabBar();
  onSwitchToPage(index);

  m_pageViewer->update();

  if (m_viewType == CLEANUP_PALETTE) updatePaletteToolBar();

  changeWindowTitle();
}

//-----------------------------------------------------------------------------

void PaletteViewer::onPaletteSwitched() {
  updateView();

  int pageIndex = 0;
  if (m_paletteHandle) {
    TPalette *palette = m_paletteHandle->getPalette();
    if (palette) {
      int currentStyleId   = palette->getCurrentStyleId();
      TPalette::Page *page = palette->getStylePage(currentStyleId);
      if (page) pageIndex  = page->getIndex();
    }
  }
  onSwitchToPage(pageIndex);

  // update GUI according to the "lock" property
  if (getPalette() && m_viewType != CLEANUP_PALETTE &&
      (m_lockPaletteAction || m_lockPaletteToolButton)) {
    if (m_lockPaletteAction) {
      m_lockPaletteAction->setEnabled(true);
      m_lockPaletteAction->setChecked(getPalette()->isLocked());
    } else if (m_lockPaletteToolButton) {
      m_lockPaletteToolButton->setEnabled(true);
      m_lockPaletteToolButton->setChecked(getPalette()->isLocked());
    }
    // update commands
    m_pageViewer->updateCommandLocks();
  } else {
    if (m_lockPaletteAction)
      m_lockPaletteAction->setEnabled(false);
    else if (m_lockPaletteToolButton)
      m_lockPaletteToolButton->setEnabled(false);
  }
}

//-----------------------------------------------------------------------------

void PaletteViewer::onFrameSwitched() {
  TPalette *palette = getPalette();
  if (!palette) return;
  palette->setFrame(m_frameHandle->getFrameIndex());
  m_pageViewer->update();
}

//-----------------------------------------------------------------------------
/*! Set a new name to palette page of index \b tabIndex.
 */
void PaletteViewer::onTabTextChanged(int tabIndex) {
  if (!m_paletteHandle) return;
  QString newName = m_pagesBar->tabText(tabIndex);
  PaletteCmd::renamePalettePage(m_paletteHandle, tabIndex,
                                newName.toStdWString());
}

//-----------------------------------------------------------------------------
/*! Change page style view mode.
 */
void PaletteViewer::onViewMode(QAction *action) {
  int viewMode = action->data().toInt();
  m_pageViewer->setViewMode((PageViewer::ViewMode)viewMode);
}

//-----------------------------------------------------------------------------
/*!  Change name display mode on the style chips
 */
void PaletteViewer::onNameDisplayMode(QAction *action) {
  int nameDisplayMode = action->data().toInt();
  m_pageViewer->setNameDisplayMode(
      (PageViewer::NameDisplayMode)nameDisplayMode);
}

//-----------------------------------------------------------------------------
/*! If current view type is LEVEL_PALETTE add to window title current level
name and current frame.
*/
void PaletteViewer::changeWindowTitle() {
  QString name = tr("Palette");
  QWidget *titleOwner;
  TPalette *palette = getPalette();
  if (m_viewType == LEVEL_PALETTE) {
    name = tr("Level Palette: ");
    if (palette) {
      name = name + QString::fromStdWString(palette->getPaletteName());
      if (palette->getDirtyFlag()) name += QString(" *");
    }
    titleOwner = parentWidget();
  } else if (m_viewType == CLEANUP_PALETTE) {
    name       = tr("Cleanup Palette");
    titleOwner = parentWidget();
  } else if (m_viewType == STUDIO_PALETTE) {
    name = QString();
    if (palette) {
      if (palette->getDirtyFlag()) {
        name = QString("* ");
      }
      name = name + QString::fromStdWString(palette->getPaletteName()) +
             QString(" : ");
    }
    name += tr("Studio Palette");
    titleOwner = parentWidget()->parentWidget();
  }

  // add color model name, if exists
  TFilePath refImagePath = (palette) ? palette->getRefImgPath() : TFilePath();
  if (!refImagePath.isEmpty()) {
    QString cmName = tr("     (Color Model: ") +
                     QString::fromStdWString(refImagePath.getWideName()) +
                     tr(")");
    name += cmName;
  }

  titleOwner->setWindowTitle(name);
}

//-----------------------------------------------------------------------------
/*! Move palette view page from \b srcIndex page index to \b dstIndex page
 * index.
 */
void PaletteViewer::movePage(int srcIndex, int dstIndex) {
  PaletteCmd::movePalettePage(m_paletteHandle, srcIndex, dstIndex);
  onSwitchToPage(dstIndex);
}

//-----------------------------------------------------------------------------
/*! Process when the lock button toggled
 */
void PaletteViewer::setIsLocked(bool lock) {
  if (m_viewType == CLEANUP_PALETTE) return;

  getPalette()->setIsLocked(lock);
  getPalette()->setDirtyFlag(true);
  m_pageViewer->updateCommandLocks();
  // notify for updating the style editor
  m_paletteHandle->notifyPaletteLockChanged();
}

void PaletteViewer::onSwitchToPage(int pageIndex) {
  m_pagesBar->setCurrentIndex(pageIndex);
}

//-----------------------------------------------------------------------------

void PaletteViewer::onShowNewStyleButtonToggled() {
  ShowNewStyleButton = (ShowNewStyleButton == 1) ? 0 : 1;
  QAction *act       = dynamic_cast<QAction *>(sender());
  if (act) {
    QString str = (ShowNewStyleButton) ? tr("Hide New Style Button")
                                       : tr("Show New Style Button");
    act->setText(str);
  }
  m_pageViewer->computeSize();
  m_pageViewer->update();
}

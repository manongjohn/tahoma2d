#pragma once

#ifndef LAYER_FOOTER_PANEL_INCLUDED
#define LAYER_FOOTER_PANEL_INCLUDED

#include <QWidget>
#include <QSlider>
#include <QKeyEvent>
#include <boost/optional.hpp>

#include "orientation.h"
#include "xshnoteviewer.h"

using boost::optional;

class XsheetViewer;

// Panel showing column footers for layers in timeline mode
class LayerFooterPanel final : public QWidget {
  Q_OBJECT

  QString m_tooltip;
  QPoint m_pos;

  QSlider *m_frameZoomSlider;

  bool isCtrlPressed        = false;
  bool m_zoomInHighlighted  = false;
  bool m_zoomOutHighlighted = false;
  // bool m_addLevelHighlighted = false;

private:
  XsheetViewer *m_viewer;

public:
  LayerFooterPanel(XsheetViewer *viewer, QWidget *parent = 0,
                   Qt::WindowFlags flags = Qt::WindowFlags());
  ~LayerFooterPanel();

  void showOrHide(const Orientation *o);

  void setZoomSliderValue(int val);

  void onControlPressed(bool pressed);
  const bool isControlPressed();

  XsheetGUI::FooterNoteArea *m_noteArea;

protected:
  void paintEvent(QPaintEvent *event) override;

  void enterEvent(QEnterEvent *) override;
  void leaveEvent(QEvent *) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  bool event(QEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *) override;

  void keyPressEvent(QKeyEvent *event) override { event->ignore(); }
  void wheelEvent(QWheelEvent *event) override { event->ignore(); }

public slots:
  void onFrameZoomSliderValueChanged(int val);
  void onFramesPerPageSelected();
};
#endif
#pragma once

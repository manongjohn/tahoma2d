#pragma once

#ifndef LIPSYNCPOPUP_H
#define LIPSYNCPOPUP_H

#include "toonzqt/dvdialog.h"
#include "toonzqt/filefield.h"
#include "toonz/txshlevel.h"
#include "toonzqt/intfield.h"
#include "toonzqt/tabbar.h"
#include "toonzqt/gutil.h"

#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QMediaPlayer>

// forward declaration
class QLabel;
class TXshSimpleLevel;
class TXshChildLevel;
class TFrameId;
class QComboBox;
class QTextEdit;
class QIcon;
class QProcess;
class QGroupBox;
class QStackedWidget;

//=============================================================================
// LipSyncPopup
//-----------------------------------------------------------------------------

class LipSyncPopup final : public DVGui::Dialog {
  Q_OBJECT

  QLabel *m_aiLabel;
  QLabel *m_oLabel;
  QLabel *m_eLabel;
  QLabel *m_uLabel;
  QLabel *m_lLabel;
  QLabel *m_wqLabel;
  QLabel *m_mbpLabel;
  QLabel *m_fvLabel;
  QLabel *m_restLabel;
  QLabel *m_otherLabel;
  QComboBox *m_soundLevels;
  QTextEdit *m_scriptEdit;
  QIcon m_playIcon, m_stopIcon;

  QLabel *m_imageLabels[10];
  QLabel *m_textLabels[10];
  QPushButton *m_navButtons[20];
  QPixmap m_pixmaps[10];
  QPushButton *m_applyButton;
  QPushButton *m_playButton;
  std::vector<TFrameId> m_levelFrameIds;
  std::vector<TFrameId> m_activeFrameIds;
  DVGui::FileField *m_file;
  DVGui::FileField *m_audioFile;
  TXshSimpleLevel *m_sl;
  TXshChildLevel *m_cl;
  TXshLevelP m_childLevel;
  DVGui::IntLineEdit *m_startAt;
  int m_col;
  int m_startFrame = -1;
  bool m_valid     = false;
  bool m_isEditingLevel;
  QStringList m_textLines;
  QCheckBox *m_restToEnd;
  QString m_audioPath;
  TFilePath m_datPath;
  QMediaPlayer *m_player;
  QLabel *m_scriptLabel;
  QLabel *m_columnLabel;
  QLabel *m_insertAtLabel;
  QGroupBox *m_rhubarbBox;
  bool m_deleteFile = false;
  DVGui::ProgressDialog *m_progressDialog;
  QProcess *m_rhubarb;
  QString m_rhubarbPath;
  QFrame *m_audioFrame;
  QFrame *m_dataFrame;
  QStackedWidget *m_stackedChooser;
  TabBarContainter *m_tabBarContainer;  //!< Tabs container for pages
  DVGui::TabBar *m_tabBar;
  QLabel *m_recognizerLabel;
  QComboBox *m_recognizer;

public:
  LipSyncPopup();

protected:
  void showEvent(QShowEvent *) override;
  void paintEvent(QPaintEvent *) override;
  void refreshSoundLevels();
  void saveAudio();
  void runRhubarb();

public slots:
  void onApplyButton();
  void imageNavClicked(int id);
  void onStartValueChanged();
  void playSound();
  bool setAudioFile();
  void onLevelChanged(int);
  void onMediaStateChanged(QMediaPlayer::State state);
  void onOutputReady();
  void setPage(int);
};

#endif  // LIPSYNCPOPUP_H

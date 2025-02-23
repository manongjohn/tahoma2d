#pragma once

#ifdef TOONZSCRIPTING

#ifndef SCRIPTBINDING_LEVEL_H
#define SCRIPTBINDING_LEVEL_H

#include "toonz/scriptbinding.h"
#include "toonz/scriptbinding_image.h"

class ToonzScene;
class TXshSimpleLevel;

namespace TScriptBinding {

class DVAPI Level final : public Wrapper {
  Q_OBJECT
  TXshSimpleLevel *m_sl;
  ToonzScene *m_scene;
  bool m_sceneOwner;
  int m_type;

public:
  Level();
  Level(TXshSimpleLevel *);
  ~Level();

  WRAPPER_STD_METHODS(Image)
  Q_INVOKABLE QJSValue toString();

  // QJSValue toScriptValue(QJSEngine *engine) { return create(engine,
  // this); }
  // static QJSValue toScriptValue(QJSEngine *engine, TXshSimpleLevel
  // *sl) { return (new Level(sl))->toScriptValue(engine); }

  Q_PROPERTY(QString type READ getType)
  QString getType() const;

  Q_PROPERTY(int frameCount READ getFrameCount)
  int getFrameCount() const;

  Q_PROPERTY(QString name READ getName WRITE setName)
  QString getName() const;
  void setName(const QString &name);

  Q_PROPERTY(QJSValue path READ getPath WRITE setPath)
  QJSValue getPath() const;
  void setPath(const QJSValue &name);

  Q_INVOKABLE QJSValue getFrame(const QJSValue &fid);
  Q_INVOKABLE QJSValue getFrameByIndex(const QJSValue &index);
  Q_INVOKABLE QJSValue setFrame(const QJSValue &fid,
                                    const QJSValue &image);

  Q_INVOKABLE QJSValue getFrameIds();

  Q_INVOKABLE QJSValue load(const QJSValue &fp);
  Q_INVOKABLE QJSValue save(const QJSValue &fp);

  void getFrameIds(QList<TFrameId> &fids);
  TImageP getImg(const TFrameId &fid);

  int setFrame(const TFrameId &fid, const TImageP &img);

  TXshSimpleLevel *getSimpleLevel() const { return m_sl; }

  static TFrameId getFid(const QJSValue &arg, QString &err);
};

}  // namespace TScriptBinding

Q_DECLARE_METATYPE(TScriptBinding::Level *)

#endif

#endif  // TOONZSCRIPTING

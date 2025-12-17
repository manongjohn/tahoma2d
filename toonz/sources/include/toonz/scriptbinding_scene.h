#pragma once

#ifdef TOONZSCRIPTING

#ifndef SCRIPTBINDING_SCENE_H
#define SCRIPTBINDING_SCENE_H

#include "toonz/scriptbinding.h"

namespace TScriptBinding {

class DVAPI Scene final : public Wrapper {
  Q_OBJECT
  ToonzScene *m_scene;

public:
  Scene();
  ~Scene();

  WRAPPER_STD_METHODS(Scene)
  Q_INVOKABLE QJSValue toString();

  Q_PROPERTY(int frameCount READ getFrameCount)
  Q_PROPERTY(int columnCount READ getColumnCount)
  int getFrameCount();
  int getColumnCount();

  Q_INVOKABLE QJSValue load(const QJSValue &fpArg);
  Q_INVOKABLE QJSValue save(const QJSValue &fpArg);

  QString doSetCell(int row, int col, const QJSValue &level,
                    const QJSValue &fid);
  Q_INVOKABLE QJSValue setCell(int row, int col, const QJSValue &level,
                                   const QJSValue &fid);
  Q_INVOKABLE QJSValue setCell(int row, int col, const QJSValue &cell);
  Q_INVOKABLE QJSValue getCell(int row, int col);

  Q_INVOKABLE QJSValue insertColumn(int col);
  Q_INVOKABLE QJSValue deleteColumn(int col);

  Q_INVOKABLE QJSValue getLevels() const;
  Q_INVOKABLE QJSValue getLevel(const QString &name) const;
  Q_INVOKABLE QJSValue newLevel(const QString &type,
                                    const QString &name) const;
  Q_INVOKABLE QJSValue loadLevel(const QString &name,
                                     const QJSValue &path) const;

  ToonzScene *getToonzScene() const { return m_scene; }
};

}  // namespace TScriptBinding

Q_DECLARE_METATYPE(TScriptBinding::Scene *)

#endif

#endif  // TOONZSCRIPTING

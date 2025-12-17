#pragma once

#ifdef TOONZSCRIPTING

#ifndef SCRIPTBINDING_RENDERER_H
#define SCRIPTBINDING_RENDERER_H

#include "toonz/scriptbinding.h"

namespace TScriptBinding {

class DVAPI Renderer final : public Wrapper {
  Q_OBJECT
  class Imp;
  Imp *m_imp;

public:
  Renderer();
  ~Renderer();

  WRAPPER_STD_METHODS(Renderer)
  Q_INVOKABLE QJSValue toString();

  Q_INVOKABLE QJSValue renderScene(const QJSValue &scene);
  Q_INVOKABLE QJSValue renderFrame(const QJSValue &scene, int frame);

  Q_INVOKABLE void dumpCache();
};

}  // namespace TScriptBinding

Q_DECLARE_METATYPE(TScriptBinding::Renderer *)

#endif

#endif  // TOONZSCRIPTING

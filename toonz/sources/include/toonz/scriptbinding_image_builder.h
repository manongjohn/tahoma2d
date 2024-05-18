#pragma once

#ifndef SCRIPTBINDING_IMAGE_BUILDER_H
#define SCRIPTBINDING_IMAGE_BUILDER_H

#include "toonz/scriptbinding.h"

class ToonzScene;
class TXshSimpleLevel;

namespace TScriptBinding {

class DVAPI Transform final : public Wrapper {
  Q_OBJECT
  TAffine m_affine;

public:
  Transform();
  Transform(const TAffine &aff);

  ~Transform();

  WRAPPER_STD_METHODS(Transform)
  Q_INVOKABLE QJSValue toString();

  Q_INVOKABLE QJSValue translate(double x, double y);
  Q_INVOKABLE QJSValue rotate(double degrees);
  Q_INVOKABLE QJSValue scale(double s);
  Q_INVOKABLE QJSValue scale(double sx, double sy);

  const TAffine &getAffine() const { return m_affine; }
};

class DVAPI ImageBuilder final : public Wrapper {
  Q_OBJECT
  TImageP m_img;
  int m_width, m_height;

  QString add(const TImageP &img, const TAffine &aff);

public:
  ImageBuilder();
  ~ImageBuilder();

  WRAPPER_STD_METHODS(ImageBuilder)
  Q_INVOKABLE QJSValue toString();

  Q_PROPERTY(QJSValue image READ getImage)
  QJSValue getImage();

  Q_INVOKABLE QJSValue add(QJSValue img);
  Q_INVOKABLE QJSValue add(QJSValue img, QJSValue transformation);

  Q_INVOKABLE void clear();
  Q_INVOKABLE QJSValue fill(const QString &colorName);
};

}  // namespace TScriptBinding

Q_DECLARE_METATYPE(TScriptBinding::Transform *)
Q_DECLARE_METATYPE(TScriptBinding::ImageBuilder *)

#endif

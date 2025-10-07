
#ifdef TOONZSCRIPTING

#include "toonz/scriptbinding.h"
#include "toonz/scriptbinding_scene.h"
#include "toonz/scriptbinding_image.h"
#include "toonz/scriptbinding_level.h"
#include "toonz/scriptbinding_files.h"
#include "toonz/scriptbinding_renderer.h"
#include "toonz/scriptbinding_outline_vectorizer.h"
#include "toonz/scriptbinding_centerline_vectorizer.h"
#include "toonz/scriptbinding_rasterizer.h"
#include "toonz/scriptbinding_toonz_raster_converter.h"
#include "toonz/scriptbinding_image_builder.h"
#include <QJSEngine>
#include "tlevel_io.h"
#include "tlevel.h"
#include "toonz/tcenterlinevectorizer.h"
#include "toonz/tcamera.h"
#include "trop.h"
#include "trasterimage.h"
#include "ttoonzimage.h"
#include "tvectorimage.h"
#include "tpalette.h"
#include "tofflinegl.h"
#include "tvectorrenderdata.h"
#include "tgeometry.h"
#include "toonz/stage.h"
#include "toonz/txshleveltypes.h"
#include "toonz/levelproperties.h"
#include "toonz/toonzscene.h"
#include "tfiletype.h"
#include <QRegularExpression>
#include <QColor>

namespace TScriptBinding {

//===========================================================================

WRAPPER_STD_CTOR_IMPL(Void)

//===========================================================================

int Wrapper::m_count = 0;

Wrapper::Wrapper() { m_id = ++m_count; }

Wrapper::~Wrapper() {}

void Wrapper::print(const QString &msg) {
  QJSValueList lst;
  lst << msg;
  print(lst);
}

void Wrapper::print(const QJSValueList &lst) {
  QJSValue print = engine()->globalObject().property("print");
  print.call(print, lst);
}

void Wrapper::warning(const QString &msg) {
  QJSValueList lst;
  lst << msg;
  QJSValue f = engine()->globalObject().property("warning");
  f.call(f, lst);
}

//===========================================================================

QJSValue checkArgumentCount(QScriptContext *context, const QString &name,
                                int minCount, int maxCount) {
  int count = context->argumentCount();
  if (minCount <= count && count <= maxCount) return QJSValue();
  QString range;
  if (minCount != maxCount)
    range = QObject::tr("%1-%2").arg(minCount).arg(maxCount);
  else
    range = QObject::tr("%1").arg(minCount);
  return context->throwError(
      QObject::tr("Expected %1 argument(s) in %2, got %3")
          .arg(range)
          .arg(name)
          .arg(count));
}

QJSValue checkArgumentCount(QScriptContext *context, const QString &name,
                                int count) {
  return checkArgumentCount(context, name, count, count);
}

QJSValue checkColor(QScriptContext *context, const QString &colorName,
                        QColor &color) {
  color.setNamedColor(colorName);
  if (!color.isValid())
    return context->throwError(
        QObject::tr("%1 is not a valid color (valid color names are 'red', "
                    "'transparent', '#FF8800', ecc.)")
            .arg(colorName));
  else
    return QJSValue();
}

//=============================================================================

template <class T>
void bindClass(QJSEngine &engine, const QString &name) {
  const QMetaObject *metaObj = &T::staticMetaObject;
  QJSValue ctor          = engine.newFunction(T::ctor);
  QJSValue proto         = engine.newQMetaObject(metaObj, ctor);
  engine.globalObject().setProperty(name, proto);
  engine.setDefaultPrototype(qMetaTypeId<T *>(), proto);
}

template <typename Tp>
QJSValue qScriptValueFromQObject(QJSEngine *engine, Tp const &qobject) {
  return engine->newQObject(qobject);
}

template <typename Tp>
void qScriptValueToQObject(const QJSValue &value, Tp &qobject) {
  qobject = qobject_cast<Tp>(value.toQObject());
}

template <typename Tp>
int qScriptRegisterQObjectMetaType(
    QJSEngine *engine, const QJSValue &prototype = QJSValue()) {
  return qScriptRegisterMetaType<Tp>(engine, qScriptValueFromQObject,
                                     qScriptValueToQObject, prototype);
}

template <class T, QJSValue (T::*Method)(QScriptContext *, QJSEngine *)>
class Dummy {
public:
  static QJSValue dummy(QScriptContext *context, QJSEngine *engine) {
    T *obj = qscriptvalue_cast<T *>(context->thisObject());
    return (obj->*Method)(context, engine);
  }
};

static void deffoo(QJSEngine &engine) {
  QJSValue f = engine.newFunction(
      Dummy<ToonzRasterConverter, &ToonzRasterConverter::convert>::dummy);
  engine.globalObject()
      .property("ToonzRasterConverter")
      .setProperty("convert", f);
}

void bindAll(QJSEngine &engine) {
  bindClass<Image>(engine, "Image");
  bindClass<Level>(engine, "Level");
  bindClass<Scene>(engine, "Scene");
  bindClass<Transform>(engine, "Transform");
  bindClass<ImageBuilder>(engine, "ImageBuilder");
  bindClass<OutlineVectorizer>(engine, "OutlineVectorizer");
  bindClass<CenterlineVectorizer>(engine, "CenterlineVectorizer");
  bindClass<Rasterizer>(engine, "Rasterizer");
  bindClass<ToonzRasterConverter>(engine, "ToonzRasterConverter");
  deffoo(engine);
  bindClass<FilePath>(engine, "FilePath");
  bindClass<Renderer>(engine, "Renderer");

  qScriptRegisterQObjectMetaType<TScriptBinding::Image *>(&engine);

  engine.evaluate("ToonzVersion='7.1'");
  // engine.evaluate("toonz={version:'7.0', }; script={version:'1.0'};");
  // engine.globalObject().setProperty("dir","C:\\Users\\gmt\\GMT to
  // vectorize\\");
  // QJSValue test = engine.evaluate("function() { print('ok');
  // run(dir+'bu.js');}");
  // engine.globalObject().setProperty("test",test);
}

}  // namespace TScriptBinding

#endif  // TOONZSCRIPTING

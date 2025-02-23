#pragma once

#ifdef TOONZSCRIPTING

#ifndef SCRIPTENGINE_H
#define SCRIPTENGINE_H

// #define SCRIPTENGINE_TEST

#include "tcommon.h"
#undef DVAPI
#undef DVVAR
#ifdef TOONZLIB_EXPORTS
#define DVAPI DV_EXPORT_API
#define DVVAR DV_EXPORT_VAR
#else
#define DVAPI DV_IMPORT_API
#define DVVAR DV_IMPORT_VAR
#endif

#include <QObject>
#include <QMetaType>

class Foo;
class QJSValue;
class QJSEngine;

class DVAPI ScriptCommand {
public:
  ScriptCommand() {}
  virtual ~ScriptCommand() {}

  virtual void execute() = 0;
};

class DVAPI ScriptEngine final : public QObject {
  Q_OBJECT
  QJSEngine *m_engine;
  class Executor;
  friend class Executor;
  Executor *m_executor;
  class MainThreadEvaluationData;
  MainThreadEvaluationData *m_mainThreadEvaluationData;
  QJSValue *m_voidValue;

public:
  ScriptEngine();
  ~ScriptEngine();

  void evaluate(const QString &cmd);
  void interrupt();
  bool isEvaluating() const;
  bool wait(unsigned long time = ULONG_MAX);

  enum OutputType {
    SimpleText,
    Warning,
    ExecutionError,
    SyntaxError,
    EvaluationResult,
    UndefinedEvaluationResult
  };
  void emitOutput(OutputType type, const QString &value) {
    emit output((int)type, value);
  }

  const QJSValue &evaluateOnMainThread(const QJSValue &fun,
                                           const QJSValue &arguments);

  QJSEngine *getQJSEngine() const { return m_engine; }
  const QJSValue &voidValue() const { return *m_voidValue; }

protected slots:
  void onTerminated();
  void onMainThreadEvaluationPosted();

signals:
  void evaluationDone();
  void output(int type, const QString &);

  void mainThreadEvaluationPosted();
};

Q_DECLARE_METATYPE(ScriptEngine *)

#endif

#endif  // TOONZSCRIPTING

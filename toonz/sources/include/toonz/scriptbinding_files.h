#pragma once

#ifdef TOONZSCRIPTING

#ifndef SCRIPTBINDING_FILES_H
#define SCRIPTBINDING_FILES_H

#include "toonz/scriptbinding.h"
#include "tfilepath.h"

#include <QDateTime>

namespace TScriptBinding {

class DVAPI FilePath final : public Wrapper {
  Q_OBJECT
  QString m_filePath;

public:
  FilePath(const QString &filePath = "");
  FilePath(const TFilePath &filePath);
  ~FilePath();

  WRAPPER_STD_METHODS(FilePath)
  Q_INVOKABLE QJSValue toString() const;

  Q_PROPERTY(QString extension READ getExtension WRITE setExtension)
  QString getExtension() const;
  QJSValue setExtension(const QString &extension);

  Q_PROPERTY(QString name READ getName WRITE setName)
  QString getName() const;
  void setName(const QString &name);

  Q_PROPERTY(QJSValue parentDirectory READ getParentDirectory WRITE
                 setParentDirectory)
  QJSValue getParentDirectory() const;
  void setParentDirectory(const QJSValue &name);

  Q_PROPERTY(bool exists READ exists)
  bool exists() const;

  Q_PROPERTY(QDateTime lastModified READ lastModified)
  QDateTime lastModified() const;

  Q_PROPERTY(bool isDirectory READ isDirectory)
  bool isDirectory() const;

  Q_INVOKABLE QJSValue withExtension(const QString &extension);
  Q_INVOKABLE QJSValue withName(const QString &extension);
  Q_INVOKABLE QJSValue
  withParentDirectory(const QJSValue &parentDirectory);

  TFilePath getToonzFilePath() const;

  Q_INVOKABLE QJSValue concat(const QJSValue &value) const;

  // return a list of FilePath contained in the folder (assuming this FilePath
  // is a folder)
  Q_INVOKABLE QJSValue files() const;
};

// helper functions

// convert a string or a FilePath object into a TFilePath
// if no conversion is possible it returns an error object, else it returns
// QJSValue() and assign the conversion result to fp
QJSValue checkFilePath(QScriptContext *context, const QJSValue &value,
                           TFilePath &fp);

}  // namespace TScriptBinding

Q_DECLARE_METATYPE(TScriptBinding::FilePath *)

#endif

#endif  // TOONZSCRIPTING



#include "versioncontrolxmlreader.h"

// TnzBase includes
#include "permissionsmanager.h"

// Qt includes
#include <QStringList>

//=============================================================================
// SVNPartialLockReader
//-----------------------------------------------------------------------------

SVNPartialLockReader::SVNPartialLockReader(const QString &xmlSVNResponse)
    : m_data(xmlSVNResponse) {
  addData(m_data);

  while (!atEnd()) {
    readNext();

    if (isStartElement()) {
      if (name().toString() == "target") readTarget();
    }
  }
}

//-----------------------------------------------------------------------------

void SVNPartialLockReader::readTarget() {
  Q_ASSERT(isStartElement() && name().toString() == "target");

  QString fileName = attributes().value("path").toString();

  m_currentPartialLock            = SVNPartialLock();
  m_currentPartialLock.m_fileName = fileName;

  while (!atEnd()) {
    readNext();

    if (isEndElement()) break;
    if (isStartElement()) {
      if (name().toString() == "property") {
        readProperty();
      } else
        readUnknownElement();
    }
  }
}

//-----------------------------------------------------------------------------

void SVNPartialLockReader::readProperty() {
  Q_ASSERT(isStartElement() && name().toString() == "property");

  if (attributes().value("name").toString() == "partial-lock") {
    QString text = readElementText();
    readPartialLock(text);
  }
  readNext();
}

//-----------------------------------------------------------------------------

void SVNPartialLockReader::readUnknownElement() {
  Q_ASSERT(isStartElement());

  while (!atEnd()) {
    readNext();

    if (isEndElement()) break;

    if (isStartElement()) readUnknownElement();
  }
}

//-----------------------------------------------------------------------------

void SVNPartialLockReader::readPartialLock(const QString &text) {
  QStringList entries = text.split(";");
  QList<SVNPartialLockInfo> lockList;

  int count = entries.size();
  for (int i = 0; i < count; i++) {
    QString entry      = entries.at(i);
    QStringList values = entry.split(":");
    if (values.size() == 3) {
      QString identifier   = values.at(0);
      QStringList userData = identifier.split("@");

      if (userData.size() == 2) {
        SVNPartialLockInfo lockInfo;
        lockInfo.m_userName = userData.at(0);
        lockInfo.m_hostName = userData.at(1);
        lockInfo.m_from     = values.at(1).toInt();
        lockInfo.m_to       = values.at(2).toInt();

        lockList.append(lockInfo);
      }
    }
  }
  m_currentPartialLock.m_partialLockList = lockList;
  m_partialLock.append(m_currentPartialLock);
}

//=============================================================================
// SVNConfigReader
//-----------------------------------------------------------------------------

SVNConfigReader::SVNConfigReader(const QString &xml) : m_data(xml), m_path() {
  addData(m_data);

  while (!atEnd()) {
    readNext();

    if (isStartElement()) {
      if (name().toString() == "repository")
        readRepository();
      else if (name().toString() == "svnPath")
        readSVNPath();
    }
  }
}

//-----------------------------------------------------------------------------

void SVNConfigReader::readRepository() {
  Q_ASSERT(isStartElement() && name().toString() == "repository");

  QString repoName, repoPath, localPath, username, password;

  while (!atEnd()) {
    readNext();

    if (isEndElement()) break;
    if (isStartElement()) {
      if (name().toString() == "name") {
        repoName = readElementText();
        readNext();
      } else if (name().toString() == "localPath") {
        localPath = readElementText();
        readNext();
      } else if (name().toString() == "repoPath") {
        repoPath = readElementText();
        readNext();
      } else
        readUnknownElement();
    }
  }

  SVNRepository repo;
  repo.m_name      = repoName;
  repo.m_localPath = localPath;
  repo.m_repoPath  = repoPath;
  repo.m_username  = QString::fromStdString(
      PermissionsManager::instance()->getSVNUserName(m_repositories.size()));
  repo.m_password = QString::fromStdString(
      PermissionsManager::instance()->getSVNPassword(m_repositories.size()));
  m_repositories.append(repo);
}

//-----------------------------------------------------------------------------

void SVNConfigReader::readSVNPath() {
  Q_ASSERT(isStartElement() && name().toString() == "svnPath");
  m_path = readElementText();
  readNext();
}

//-----------------------------------------------------------------------------

void SVNConfigReader::readUnknownElement() {
  Q_ASSERT(isStartElement());

  while (!atEnd()) {
    readNext();

    if (isEndElement()) break;

    if (isStartElement()) readUnknownElement();
  }
}

//=============================================================================
// SVNStatusReader
//-----------------------------------------------------------------------------

SVNStatusReader::SVNStatusReader(const QString &xmlSVNResponse)
    : m_data(xmlSVNResponse) {
  addData(m_data);

  resetCurrentValues();

  while (!atEnd()) {
    readNext();

    if (isStartElement()) {
      if (name().toString() == "entry") readEntry();
    }
  }
}

//-----------------------------------------------------------------------------

SVNStatusReader::~SVNStatusReader() { m_status.clear(); }

//-----------------------------------------------------------------------------

void SVNStatusReader::resetCurrentValues() {
  m_currentPath           = "";
  m_currentCommitRevision = "";
  m_currentAuthor         = "";
  m_currentDate           = "";
  m_currentIsLocked       = false;
  m_currentLockComment    = "";
  m_currentLockDate       = "";
  m_currentLockOwner      = "";
}

//-----------------------------------------------------------------------------

void SVNStatusReader::readEntry() {
  Q_ASSERT(isStartElement() && name().toString() == "entry");

  m_currentPath = attributes().value("path").toString();

  while (!atEnd()) {
    readNext();

    if (isEndElement()) break;
    if (isStartElement()) {
      if (name().toString() == "wc-status") {
        readWCStatus();
      } else if (name().toString() == "repos-status") {
        readRepoStatus();
      } else
        readUnknownElement();
    }
  }

  resetCurrentValues();
}

//-----------------------------------------------------------------------------

void SVNStatusReader::readRepoStatus() {
  Q_ASSERT(isStartElement() && name().toString() == "repos-status");

  QString props = attributes().value("props").toString();
  QString item  = attributes().value("item").toString();

  while (!atEnd()) {
    readNext();

    if (isEndElement()) break;
    if (isStartElement()) {
      if (name().toString() == "lock") {
        readLock(true);
      } else
        readUnknownElement();
    }
  }

  if (props == "none")
    m_status.last().m_repoStatus = item;
  else
    m_status.last().m_repoStatus = props;
}

//-----------------------------------------------------------------------------

void SVNStatusReader::readWCStatus() {
  Q_ASSERT(isStartElement() && name().toString() == "wc-status");

  QString props = attributes().value("props").toString();
  QString item  = attributes().value("item").toString();

  QString revision;
  if (attributes().hasAttribute("revision"))
    revision = attributes().value("revision").toString();

  while (!atEnd()) {
    readNext();

    if (isEndElement()) break;
    if (isStartElement()) {
      if (name().toString() == "commit") {
        readCommit();
      } else if (name().toString() == "lock") {
        readLock(false);
      } else
        readUnknownElement();
    }
  }

  SVNStatus status;
  status.m_isLocked        = false;
  status.m_item            = item;
  status.m_workingRevision = revision;
  status.m_commitRevision  = m_currentCommitRevision;
  status.m_author          = m_currentAuthor;
  status.m_path            = m_currentPath;
  status.m_commitDate      = m_currentDate;

  // Split the lock comment to retrieve the lock hostName
  int separatorPos = m_currentLockComment.indexOf(":");
  if (separatorPos == -1) {
    status.m_lockComment  = m_currentLockComment;
    status.m_lockHostName = "";
  } else {
    status.m_lockComment  = m_currentLockComment.mid(separatorPos + 1);
    status.m_lockHostName = m_currentLockComment.left(separatorPos);
  }

  status.m_lockComment     = m_currentLockComment;
  status.m_lockOwner       = m_currentLockOwner;
  status.m_lockDate        = m_currentLockDate;
  status.m_isLocked        = m_currentIsLocked;
  status.m_isPartialLocked = false;
  status.m_isPartialEdited = false;
  status.m_editFrom        = 0;
  status.m_editTo          = 0;

  m_status.append(status);
}

//-----------------------------------------------------------------------------

void SVNStatusReader::readLock(bool statusAlreadyAdded) {
  Q_ASSERT(isStartElement() && name().toString() == "lock");

  if (statusAlreadyAdded)
    m_status.last().m_isLocked = true;
  else
    m_currentIsLocked = true;

  while (!atEnd()) {
    readNext();

    if (isEndElement()) break;
    if (isStartElement()) {
      if (name().toString() == "owner") {
        if (statusAlreadyAdded)
          m_status.last().m_lockOwner = readElementText();
        else
          m_currentLockOwner = readElementText();
        readNext();
      } else if (name().toString() == "comment") {
        if (statusAlreadyAdded) {
          // Split the lock comment to retrieve the lock hostName
          QString lockComment = readElementText();
          int separatorPos    = lockComment.indexOf(":");
          if (separatorPos == -1) {
            m_status.last().m_lockComment  = lockComment;
            m_status.last().m_lockHostName = "";
          } else {
            m_status.last().m_lockComment  = lockComment.mid(separatorPos + 1);
            m_status.last().m_lockHostName = lockComment.left(separatorPos);
          }
        } else
          m_currentLockComment = readElementText();
        readNext();
      } else if (name().toString() == "created") {
        if (statusAlreadyAdded)
          m_status.last().m_lockDate = readElementText();
        else
          m_currentLockDate = readElementText();
        readNext();
      } else
        readUnknownElement();
    }
  }
}

//-----------------------------------------------------------------------------

void SVNStatusReader::readCommit() {
  Q_ASSERT(isStartElement() && name().toString() == "commit");

  m_currentCommitRevision = attributes().value("revision").toString();

  while (!atEnd()) {
    readNext();

    if (isEndElement()) break;
    if (isStartElement()) {
      if (name().toString() == "author") {
        m_currentAuthor = readElementText();
        readNext();
      } else if (name().toString() == "date") {
        m_currentDate = readElementText();
        readNext();
      } else
        readUnknownElement();
    }
  }
}

//-----------------------------------------------------------------------------

void SVNStatusReader::readUnknownElement() {
  Q_ASSERT(isStartElement());

  while (!atEnd()) {
    readNext();

    if (isEndElement()) break;

    if (isStartElement()) readUnknownElement();
  }
}

//=============================================================================
// SVNLogReader
//-----------------------------------------------------------------------------

SVNLogReader::SVNLogReader(const QString &xmlSVNResponse)
    : m_data(xmlSVNResponse) {
  addData(m_data);

  while (!atEnd()) {
    readNext();

    if (isStartElement()) {
      if (name().toString() == "logentry") readEntry();
    }
  }
}
//-----------------------------------------------------------------------------

void SVNLogReader::readEntry() {
  Q_ASSERT(isStartElement() && name().toString() == "logentry");

  m_currentRevision = attributes().value("revision").toString();

  while (!atEnd()) {
    readNext();

    if (isEndElement()) break;
    if (isStartElement()) {
      if (name().toString() == "author") {
        m_currentAuthor = readElementText();
        readNext();
      } else if (name().toString() == "date") {
        m_currentDate = readElementText();
        readNext();
      } else if (name().toString() == "msg") {
        m_currentMsg = readElementText();
        readNext();
      } else
        readUnknownElement();
    }
  }

  SVNLog log;
  log.m_revision = m_currentRevision;
  log.m_author   = m_currentAuthor;
  log.m_date     = m_currentDate;
  log.m_msg      = m_currentMsg;

  m_log.append(log);
}

//-----------------------------------------------------------------------------

void SVNLogReader::readUnknownElement() {
  Q_ASSERT(isStartElement());

  while (!atEnd()) {
    readNext();

    if (isEndElement()) break;

    if (isStartElement()) readUnknownElement();
  }
}

//=============================================================================
// SVNInfoReader
//-----------------------------------------------------------------------------

SVNInfoReader::SVNInfoReader(const QString &xmlSVNResponse)
    : m_data(xmlSVNResponse) {
  addData(m_data);

  while (!atEnd()) {
    readNext();

    if (isStartElement()) {
      if (name().toString() == "entry") readEntry();
    }
  }
}

//-----------------------------------------------------------------------------

void SVNInfoReader::readEntry() {
  Q_ASSERT(isStartElement() && name().toString() == "entry");

  m_revision = attributes().value("revision").toString();

  while (!atEnd()) {
    readNext();

    if (isEndElement()) break;
    if (isStartElement()) {
      if (name().toString() == "url") {
        m_url = readElementText();
        readNext();
      } else
        readUnknownElement();
    }
  }
}

//-----------------------------------------------------------------------------

void SVNInfoReader::readUnknownElement() {
  Q_ASSERT(isStartElement());

  while (!atEnd()) {
    readNext();

    if (isEndElement()) break;

    if (isStartElement()) readUnknownElement();
  }
}

//=============================================================================
// SVNListReader
//-----------------------------------------------------------------------------

SVNListReader::SVNListReader(const QString &xmlSVNResponse)
    : m_data(xmlSVNResponse) {
  addData(m_data);

  while (!atEnd()) {
    readNext();

    if (isStartElement()) {
      if (name().toString() == "entry") readEntry();
    }
  }
}

//-----------------------------------------------------------------------------

void SVNListReader::readEntry() {
  Q_ASSERT(isStartElement() && name().toString() == "entry");

  QString entryKind = attributes().value("kind").toString();
  QString entryName = "";

  while (!atEnd()) {
    readNext();

    if (isEndElement()) break;
    if (isStartElement()) {
      if (name().toString() == "name") {
        entryName = readElementText();
        readNext();
      } else
        readUnknownElement();
    }
  }

  SVNListInfo li;
  li.m_name = entryName;
  li.m_kind = entryKind;
  m_listInfo.append(li);
}

//-----------------------------------------------------------------------------

void SVNListReader::readUnknownElement() {
  Q_ASSERT(isStartElement());

  while (!atEnd()) {
    readNext();

    if (isEndElement()) break;

    if (isStartElement()) readUnknownElement();
  }
}

//-----------------------------------------------------------------------------

QStringList SVNListReader::getDirs() {
  QStringList dirs;
  int size = m_listInfo.size();
  for (int i = 0; i < size; i++) {
    SVNListInfo li = m_listInfo.at(i);
    if (li.m_kind == "dir") dirs.append(li.m_name);
  }
  return dirs;
}

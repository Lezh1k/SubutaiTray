#include <QFile>
#include <QFileInfo>
#include "Commons.h"
#include "NotificationObserver.h"
#include "SystemCallWrapper.h"
#include "updater/ExecutableUpdater.h"

using namespace update_system;

CExecutableUpdater::CExecutableUpdater(const QString &src,
                                       const QString &dst) :
  m_src_file_str(src),
  m_dst_file_str(dst)
{
}
////////////////////////////////////////////////////////////////////////////

CExecutableUpdater::~CExecutableUpdater() {
}
////////////////////////////////////////////////////////////////////////////

void
CExecutableUpdater::replace_executables(bool was_successful_downloaded) {
  if (!was_successful_downloaded)
    return;

  QString tmp = m_dst_file_str + ".tmp";
  QFile src(m_src_file_str);
  QFile dst(m_dst_file_str);
  bool replaced = true;
  QFile::Permissions perm;
  if (dst.exists()) {
    perm = dst.permissions();
  } else {
    perm = QFile::ReadUser | QFile::WriteUser | QFile::ExeUser  |
           QFile::ReadGroup | QFile::ExeGroup |
           QFile::ReadOther | QFile::ExeOther ; // 0x0755 :)
  }

  qInfo("dst : %s", m_dst_file_str.toStdString().c_str());
  qInfo("tmp : %s", tmp.toStdString().c_str());
  qInfo("src : %s", m_src_file_str.toStdString().c_str());

  QFileInfo fi(m_dst_file_str);
  static QString dir = fi.path();
  CSystemCallWrapper::give_write_permissions(dir);

  do {    
    if (!src.exists()) {
        qCritical("Source file is missins at %s", m_src_file_str.toStdString().c_str());
        break;
    }
    if (dst.exists()) {
      QFile ftmp(tmp);
      if (ftmp.exists() && !ftmp.remove()) {
        qCritical("remove tmp file %s failed. %s",
                                              tmp.toStdString().c_str(),
                                              ftmp.errorString().toStdString().c_str());
        break;
      }

      if (!(replaced &= dst.rename(tmp))) {
        qCritical("rename %s to %s failed. %s",
                                              m_dst_file_str.toStdString().c_str(),
                                              tmp.toStdString().c_str(),
                                              dst.errorString().toStdString().c_str());
        break;
      }
    }

    if (!(replaced &= src.copy(m_dst_file_str))) {
      qCritical("copy %s to %s failed. %s",
                                            m_src_file_str.toStdString().c_str(),
                                            m_dst_file_str.toStdString().c_str(),
                                            src.errorString().toStdString().c_str());
      break;
    }

#ifndef RT_OS_WINDOWS
    if (!(replaced &= dst.setPermissions(m_dst_file_str, perm))) {
      qCritical("set permission to file %s failed", m_dst_file_str.toStdString().c_str());
      break;
    }
#endif
  } while (0);

  emit finished(replaced);
}
////////////////////////////////////////////////////////////////////////////

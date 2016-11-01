#include <QFile>
#include "Commons.h"
#include "ApplicationLog.h"
#include "NotifiactionObserver.h"
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
  QFile::Permissions perm = dst.permissions();
  bool replaced = true;

  CApplicationLog::Instance()->LogTrace("dst : %s", m_dst_file_str.toStdString().c_str());
  CApplicationLog::Instance()->LogTrace("tmp : %s", tmp.toStdString().c_str());
  CApplicationLog::Instance()->LogTrace("src : %s", m_src_file_str.toStdString().c_str());

  do {
    if (dst.exists()) {
      if (!(replaced &= dst.rename(tmp))) {
        CApplicationLog::Instance()->LogError("rename %s to %s failed",
                                              m_dst_file_str.toStdString().c_str(),
                                              tmp.toStdString().c_str());
        break;
      }
    }

    if (!(replaced &= src.copy(m_dst_file_str))) {
      CApplicationLog::Instance()->LogError("copy %s to %s failed",
                                            m_src_file_str.toStdString().c_str(),
                                            m_dst_file_str.toStdString().c_str());
      CApplicationLog::Instance()->LogError("%s",
                                            src.errorString().toStdString().c_str());
      break;
    }

#ifndef RT_OS_WINDOWS
    if (!(replaced &= dst.setPermissions(m_dst_file_str, perm))) {
      CApplicationLog::Instance()->LogError("set permission to file %s failed", m_dst_file_str.toStdString().c_str());
      break;
    }
#else
    if (dst.setPermissions(m_dst_file_str, perm)) {
      CApplicationLog::Instance()->LogError("set permission to file %s failed", m_dst_file_str.toStdString().c_str());
    }
#endif

    QFile ftmp(tmp);
    if (!ftmp.remove()) {
      CApplicationLog::Instance()->LogError("remove tmp file %s failed", tmp.toStdString().c_str());
      break;
    }
  } while (0);

  emit finished(replaced);
}
////////////////////////////////////////////////////////////////////////////
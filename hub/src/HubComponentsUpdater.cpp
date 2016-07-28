#include <QApplication>
#include <QDir>

#include "HubComponentsUpdater.h"
#include "ExecutableUpdater.h"
#include "DownloadFileManager.h"
#include "SystemCallWrapper.h"
#include "RestWorker.h"
#include "NotifiactionObserver.h"
#include "DownloadFileManager.h"
#include "ExecutableUpdater.h"
#include "libssh2/LibsshErrors.h"

static const char* P2P = "p2p";
static const char* TRAY = "tray";
const QString CHubComponentsUpdater::STR_P2P(P2P);
const QString CHubComponentsUpdater::STR_TRAY(TRAY);

static const char* tray_update_file() {
  static const char* fn =
    #if defined(RT_OS_LINUX)
      "tray_9683ecfe-1034-11e6-b626-f816544befe7";
    #elif defined(RT_OS_DARWIN)
      "tray_9683ecfe-1034-11e6-b626-f816544befe7_mac";
    #elif defined(RT_OS_WINDOWS)
      "tray_9683ecfe-1034-11e6-b626-f816544befe7.exe"
    #else
      "";
    #error "TRAY_UPDATE_FILE macros undefined"
    #endif
  return fn;
}
////////////////////////////////////////////////////////////////////////////

static const char* p2p_update_file_name() {
  static const char* fn =
    #if defined(RT_OS_LINUX)
      "p2p";
    #elif defined(RT_OS_DARWIN)
      "p2p_osx";
    #elif defined(RT_OS_WINDOWS)
      "p2p.exe"
    #else
      "";
    #error "p2p update file name undefined"
    #endif
  return fn;
}
////////////////////////////////////////////////////////////////////////////

CHubComponentsUpdater::CHubComponentsUpdater() {
  m_dct_file_atomics[STR_P2P] = file_atomic(p2p_update_file_name());
  m_dct_file_atomics[STR_TRAY] = file_atomic(tray_update_file());
}

CHubComponentsUpdater::~CHubComponentsUpdater() {

}
////////////////////////////////////////////////////////////////////////////

std::string
CHubComponentsUpdater::p2p_path() const {
  std::string p2p_path(P2P);

  if (CSettingsManager::Instance().p2p_path() != STR_P2P) {
    p2p_path = CSettingsManager::Instance().p2p_path().toStdString();
  } else {
    system_call_wrapper_error_t cr;
    if ((cr = CSystemCallWrapper::which(P2P, p2p_path)) != SCWE_SUCCESS) {
      CNotifiactionObserver::Instance()->NotifyAboutError(QString("Can't find p2p in PATH. Err : %1").arg(
                                                            CSystemCallWrapper::scwe_error_to_str(cr)));
    }
  }
  return p2p_path;
}
////////////////////////////////////////////////////////////////////////////

chue_t
CHubComponentsUpdater::update_and_replace_file(const QString &file_id,
                                               const QString &download_path,
                                               const QString &file_to_replace_path) {
  std::vector<CGorjunFileInfo> fi =
      CRestWorker::Instance()->get_gorjun_file_info(
        m_dct_file_atomics[file_id].kurjun_file_name);

  if (fi.empty()) {
    CApplicationLog::Instance()->LogError("File %s isn't presented on kurjun", file_id.toStdString().c_str());
    return CHUE_NOT_ON_KURJUN;
  }

  std::vector<CGorjunFileInfo>::iterator item = fi.begin();
  CApplicationLog::Instance()->LogInfo("Expected md5 : %s", item->md5_sum().toStdString().c_str());
  CDownloadFileManager *dm = new CDownloadFileManager(item->id(),
                                                      file_id,
                                                      download_path,
                                                      item->size());

  CExecutableUpdater *eu = new CExecutableUpdater(file_id,
                                                  download_path,
                                                  file_to_replace_path);

  connect(dm, SIGNAL(download_progress_sig(QString,qint64,qint64)),
          this, SLOT(download_file_progress_sl(QString,qint64,qint64)));
  connect(dm, SIGNAL(finished(QString)), this, SLOT(file_downloading_finished(QString)));
  connect(dm, SIGNAL(finished(QString)), eu, SLOT(replace_executables()));
  connect(eu, SIGNAL(finished(QString,bool)), this, SLOT(file_replace_finished(QString,bool)));
  connect(eu, SIGNAL(finished(QString,bool)), dm, SLOT(deleteLater()));
  connect(eu, SIGNAL(finished(QString,bool)), eu, SLOT(deleteLater()));
  dm->start_download();
  return CHUE_SUCCESS;
}
////////////////////////////////////////////////////////////////////////////


bool
CHubComponentsUpdater::tray_check_for_update() {
  if (m_dct_file_atomics[TRAY].checks != 0) return true;
  std::vector<CGorjunFileInfo> fi = CRestWorker::Instance()->get_gorjun_file_info(tray_update_file());
  if (fi.empty()) return false;

  ++m_dct_file_atomics[TRAY].checks;
  emit update_available(TRAY);
  //todo add work with MD5 checking after installers changes.
  return true;
}
////////////////////////////////////////////////////////////////////////////


chue_t CHubComponentsUpdater::tray_update() {
  if (m_dct_file_atomics[TRAY].in_progress) return CHUE_IN_PROGRESS;
  atomic_locker al(&m_dct_file_atomics[TRAY].in_progress);
  QString download_file_path = QApplication::applicationDirPath() +
                          QDir::separator() +
                          QString(tray_update_file());
  return update_and_replace_file(tray_update_file(),
                                 download_file_path,
                                 QApplication::applicationFilePath());
}
////////////////////////////////////////////////////////////////////////////

chue_t CHubComponentsUpdater::subutai_rh_update() {
  if (m_subutai_rh_updating) return CHUE_IN_PROGRESS;
  atomic_locker al(&m_subutai_rh_updating);

  int exit_code = 0;
  CSystemCallWrapper::run_ss_updater(CSettingsManager::Instance().rh_host().toStdString().c_str(),
                                     CSettingsManager::Instance().rh_port().toStdString().c_str(),
                                     CSettingsManager::Instance().rh_user().toStdString().c_str(),
                                     CSettingsManager::Instance().rh_pass().toStdString().c_str(),
                                     exit_code);

  if (exit_code == RLE_SUCCESS) {
    static const char* msg = "Resource host update succesfull finished";
    CNotifiactionObserver::NotifyAboutInfo(msg);
    CApplicationLog::Instance()->LogInfo(msg);
    return CHUE_SUCCESS;
  }

  QString err_msg = QString("Resource host update failed with exit code : %1").arg(exit_code);
  CNotifiactionObserver::NotifyAboutError(err_msg);
  CApplicationLog::Instance()->LogError(err_msg.toStdString().c_str());
  return CHUE_FAILED;
}
////////////////////////////////////////////////////////////////////////////

bool
CHubComponentsUpdater::p2p_check_for_update() {
  if (m_dct_file_atomics[P2P].checks != 0)
    return true;
  std::vector<CGorjunFileInfo> fi =
      CRestWorker::Instance()->get_gorjun_file_info(
        m_dct_file_atomics[P2P].kurjun_file_name);
  if (fi.empty()) return false;
  std::string str_p2p_path = p2p_path();
  if (str_p2p_path == std::string(P2P)) return false;

  QString md5_current = CCommons::FileMd5(QString::fromStdString(str_p2p_path));
  QString md5_kurjun = fi[0].md5_sum();

  if (md5_current == md5_kurjun)
    return false;

  ++m_dct_file_atomics[P2P].checks;
  emit update_available(P2P);
  return true;
}
////////////////////////////////////////////////////////////////////////////

chue_t
CHubComponentsUpdater::p2p_update() {
  if (m_dct_file_atomics[P2P].in_progress) return CHUE_IN_PROGRESS;
  atomic_locker al(&m_dct_file_atomics[P2P].in_progress);
  QString str_p2p_path = QString::fromStdString(p2p_path());
  return update_and_replace_file(STR_P2P,
                                 QApplication::applicationDirPath() + QDir::separator() + STR_P2P,
                                 str_p2p_path);
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::download_file_progress_sl(QString file_id, qint64 cur,
                                                 qint64 full) {
  emit download_file_progress(file_id, cur, full);
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::file_downloading_finished(QString file_id) {
  --m_dct_file_atomics[file_id].checks;
}
////////////////////////////////////////////////////////////////////////////

void
CHubComponentsUpdater::file_replace_finished(QString file_id, bool replaced) {
  emit updating_finished(file_id, replaced);
}
////////////////////////////////////////////////////////////////////////////
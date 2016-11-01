#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include "updater/UpdaterComponentP2P.h"
#include "updater/ExecutableUpdater.h"
#include "RestWorker.h"
#include "SystemCallWrapper.h"
#include "NotifiactionObserver.h"
#include "DownloadFileManager.h"

using namespace update_system;

static const char* p2p_kurjun_file_name() {
  static const char* fn =
    #if defined(RT_OS_LINUX)
      "p2p";
    #elif defined(RT_OS_DARWIN)
      "p2p_osx";
    #elif defined(RT_OS_WINDOWS)
      "p2p.exe";
    #else
      "";
    #error "p2p update file name undefined"
    #endif
  return fn;
}
////////////////////////////////////////////////////////////////////////////

CUpdaterComponentP2P::CUpdaterComponentP2P() {
  m_component_id = QString(P2P);
}

CUpdaterComponentP2P::~CUpdaterComponentP2P() {

}
////////////////////////////////////////////////////////////////////////////

std::string CUpdaterComponentP2P::p2p_path()
{
  std::string p2p_path = P2P.toStdString();

  if (CSettingsManager::Instance().p2p_path() != P2P) {
    p2p_path = CSettingsManager::Instance().p2p_path().toStdString();
  } else {
    system_call_wrapper_error_t cr;
    if ((cr = CSystemCallWrapper::which(P2P.toStdString(), p2p_path)) != SCWE_SUCCESS) {
      CNotificationObserver::Instance()->NotifyAboutError(QString("Can't find p2p in PATH. Err : %1").arg(
                                                            CSystemCallWrapper::scwe_error_to_str(cr)));
    }
  }
  return p2p_path;
}
////////////////////////////////////////////////////////////////////////////

bool
CUpdaterComponentP2P::update_available_internal() {
  std::vector<CGorjunFileInfo> fi =
      CRestWorker::Instance()->get_gorjun_file_info(p2p_kurjun_file_name());
  if (fi.empty()) return false;
  std::string str_p2p_path = p2p_path();
  if (str_p2p_path == P2P.toStdString()) return false;
  QString md5_current = CCommons::FileMd5(QString::fromStdString(str_p2p_path));
  QString md5_kurjun = fi[0].md5_sum();
  return md5_current != md5_kurjun;
}
////////////////////////////////////////////////////////////////////////////

chue_t
CUpdaterComponentP2P::update_internal() {
  std::string str_p2p_path = p2p_path();
  if (str_p2p_path.empty()) return CHUE_FAILED;

  //original file path
  QString str_p2p_executable_path = QString::fromStdString(p2p_path());

  //this file will replace original file
  QString str_p2p_downloaded_path = QApplication::applicationDirPath() +
                                  QDir::separator() +
                                  QString(P2P);

  std::vector<CGorjunFileInfo> fi = CRestWorker::Instance()->get_gorjun_file_info(
                                      p2p_kurjun_file_name());

  if (fi.empty()) {
    CApplicationLog::Instance()->LogError("File %s isn't presented on kurjun", m_component_id.toStdString().c_str());
    return CHUE_NOT_ON_KURJUN;
  }

  std::vector<CGorjunFileInfo>::iterator item = fi.begin();
  CDownloadFileManager *dm = new CDownloadFileManager(item->id(),
                                                      str_p2p_downloaded_path,
                                                      item->size());

  CExecutableUpdater *eu = new CExecutableUpdater(str_p2p_downloaded_path,
                                                  str_p2p_executable_path);

  connect(dm, SIGNAL(download_progress_sig(qint64,qint64)),
          this, SLOT(update_progress_sl(qint64,qint64)));

  connect(dm, SIGNAL(finished(bool)), eu, SLOT(replace_executables(bool)));
  connect(eu, SIGNAL(finished(bool)), this, SLOT(update_finished_sl(bool)));
  connect(eu, SIGNAL(finished(bool)), dm, SLOT(deleteLater()));
  connect(eu, SIGNAL(finished(bool)), eu, SLOT(deleteLater()));

  dm->start_download();
  return CHUE_SUCCESS;
}
////////////////////////////////////////////////////////////////////////////

void
CUpdaterComponentP2P::update_post_action() {
  CNotificationObserver::Instance()->NotifyAboutInfo("P2P has been updated");
  int rse_err = 0;
  system_call_wrapper_error_t scwe =
      CSystemCallWrapper::restart_p2p_service(&rse_err);
  if (scwe != SCWE_SUCCESS) {
    CNotificationObserver::Instance()->NotifyAboutError(QString("p2p post update failed. err : ").
                                                        arg(CSystemCallWrapper::scwe_error_to_str(scwe)));
    return;
  }

  if (rse_err == RSE_MANUAL) {
    QMessageBox *msg_box = new QMessageBox(QMessageBox::Question, "Attention! P2P update finished",
                                           "P2P has been updated. Restart p2p daemon, please",
                                           QMessageBox::Ok);
    connect(msg_box, SIGNAL(finished(int)), msg_box, SLOT(deleteLater()));
    msg_box->exec();
  }
}
////////////////////////////////////////////////////////////////////////////
#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include <QStandardPaths>

#include "Commons.h"
#include "DownloadFileManager.h"
#include "NotificationObserver.h"
#include "OsBranchConsts.h"
#include "P2PController.h"
#include "RestWorker.h"
#include "SystemCallWrapper.h"
#include "updater/ExecutableUpdater.h"
#include "updater/HubComponentsUpdater.h"
#include "updater/UpdaterComponentVagrantParallels.h"

CUpdaterComponentVAGRANT_PARALLELS::CUpdaterComponentVAGRANT_PARALLELS() {
  this->m_component_id = VAGRANT_PARALLELS;
}

CUpdaterComponentVAGRANT_PARALLELS::~CUpdaterComponentVAGRANT_PARALLELS() {}

bool CUpdaterComponentVAGRANT_PARALLELS::update_available_internal() {
  QString version;
  static QString vagrant_plugin = "vagrant-parallels";
  system_call_wrapper_error_t res =
      CSystemCallWrapper::vagrant_plugin_version(version, vagrant_plugin);
  QString cloud_version =
      CRestWorker::Instance()->get_vagrant_plugin_cloud_version(vagrant_plugin);
  if (version == "undefined") return true;
  if (res != SCWE_SUCCESS) return false;
  if (cloud_version == "undefined" || cloud_version.isEmpty()) return false;
  return cloud_version > version;
}

chue_t CUpdaterComponentVAGRANT_PARALLELS::install_internal() {
  qDebug() << "Starting install vagrant parallels";

  QMessageBox *msg_box = new QMessageBox(
      QMessageBox::Information, QObject::tr("Attention!"),
      QObject::tr(
          "The Vagrant Parallel provider manage Parallels virtual machines."
          "The Vagrant Parallels provider will be installed on your machine.\n"
          "Do you want to proceed?"),
      QMessageBox::Yes | QMessageBox::No);

  QObject::connect(msg_box, &QMessageBox::finished, msg_box,
                   &QMessageBox::deleteLater);
  if (msg_box->exec() != QMessageBox::Yes) {
    install_finished_sl(false, "undefined");
    return CHUE_SUCCESS;
  }

  update_progress_sl(0, 0);
  static QString empty_string = "";
  SilentInstaller *silent_installer = new SilentInstaller(this);
  silent_installer->init(empty_string, empty_string, CC_VAGRANT_PARALLELS);
  connect(silent_installer, &SilentInstaller::outputReceived, this,
          &CUpdaterComponentVAGRANT_PARALLELS::install_finished_sl);
  silent_installer->startWork();
  return CHUE_SUCCESS;
}

chue_t CUpdaterComponentVAGRANT_PARALLELS::update_internal() {
  update_progress_sl(50, 100);
  static QString empty_string = "";

  SilentUpdater *silent_updater = new SilentUpdater(this);
  silent_updater->init(empty_string, empty_string, CC_VAGRANT_PARALLELS);

  connect(silent_updater, &SilentUpdater::outputReceived, this,
          &CUpdaterComponentVAGRANT_PARALLELS::update_finished_sl);

  silent_updater->startWork();

  return CHUE_SUCCESS;
}

chue_t CUpdaterComponentVAGRANT_PARALLELS::uninstall_internal() {
  update_progress_sl(50, 100);
  static QString empty_string = "";

  SilentUninstaller *silent_uninstaller = new SilentUninstaller(this);
  silent_uninstaller->init(empty_string, empty_string, CC_VAGRANT_PARALLELS);

  connect(silent_uninstaller, &SilentUninstaller::outputReceived, this,
          &CUpdaterComponentVAGRANT_PARALLELS::uninstall_finished_sl);

  silent_uninstaller->startWork();

  return CHUE_SUCCESS;
}

void CUpdaterComponentVAGRANT_PARALLELS::update_post_action(bool success) {
  if (!success) {
    CNotificationObserver::Instance()->Info(
        tr("Failed to update the Vagrant Parallels provider. You may try manually "
           "installing it "
           "or try again by restarting the Control Center first."),
        DlgNotification::N_NO_ACTION);
  } else {
    CNotificationObserver::Instance()->Info(
        tr("Vagrant Parallels provider has been updated successfully."),
        DlgNotification::N_NO_ACTION);
  }
}
void CUpdaterComponentVAGRANT_PARALLELS::install_post_internal(bool success) {
  if (!success) {
    CNotificationObserver::Instance()->Info(
        tr("Failed to install the Vagrant Parallels provider. You may try manually "
           "installing it "
           "or try again by restarting the Control Center first."),
        DlgNotification::N_NO_ACTION);
  } else {
    CNotificationObserver::Instance()->Info(
        tr("Vagrant Parallels provider has been installed successfully."),
        DlgNotification::N_NO_ACTION);
  }
}

void CUpdaterComponentVAGRANT_PARALLELS::uninstall_post_internal(bool success) {
  if (!success) {
    CNotificationObserver::Instance()->Info(
        tr("Failed to uninstall the Vagrant Parallels provider. You may try "
           "manually uninstalling it "
           "or try again by restarting the Control Center first."),
        DlgNotification::N_NO_ACTION);
  } else {
    CNotificationObserver::Instance()->Info(
        tr("Vagrant Parallels provider has been uninstalled successfully."),
        DlgNotification::N_NO_ACTION);
  }
}

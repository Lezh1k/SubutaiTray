#include "DlgEnvironment.h"
#include "ui_DlgEnvironment.h"
#include "P2PController.h"
#include <QToolTip>

DlgEnvironment::DlgEnvironment(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DlgEnvironment)
{
    ui->setupUi(this);
}

/////////////////////////////////////////////////////////////////////////


void DlgEnvironment::addEnvironment(const CEnvironment *env){
  for (auto cont = env->containers().begin() ; cont != env->containers().end() ; cont ++){
    addContainer(&(*cont));
    addRemoteAccess(env, &(*cont));
  }
  ui->btn_ssh_all->setEnabled(env->healthy());
}

/////////////////////////////////////////////////////////////////////////


void DlgEnvironment::addContainer(const CHubContainer *cont){
  QLabel *cont_name = new QLabel(cont->name(), this);
  QLabel *cont_ip = new QLabel(cont->ip(), this);
  QLabel *cont_rhip_port = new QLabel(cont->rh_ip() + ":" + cont->port(), this);
  cont_name->setAlignment(Qt::AlignHCenter);
  cont_ip->setAlignment(Qt::AlignHCenter);
  cont_rhip_port->setAlignment(Qt::AlignHCenter);
  cont_name->setTextInteractionFlags(Qt::TextSelectableByMouse);
  cont_ip->setTextInteractionFlags(Qt::TextSelectableByMouse);
  cont_rhip_port->setTextInteractionFlags(Qt::TextSelectableByMouse);
  ui->cont_name->addWidget(cont_name);
  ui->cont_ip_port->addWidget(cont_ip);
  ui->cont_rhip->addWidget(cont_rhip_port);
}

void DlgEnvironment::check_status(QPushButton *btn_ssh, const CEnvironment *env, const CHubContainer *cont) {
  if (!env->healthy()) {
    btn_ssh->setToolTip("Environment is unhealthy.");
    btn_ssh->setEnabled(false);
  }
  else
  if(!P2PController::Instance().join_swarm_success(env->hash())) {
    btn_ssh->setToolTip("The connection with environment is not established.");
    btn_ssh->setEnabled(false);
  }
  else
  if (!P2PController::Instance().handshake_success(env->id(), cont->id())) {
    btn_ssh->setToolTip("Container is not ready.");
    btn_ssh->setEnabled(false);
  }
  else {
    btn_ssh->setToolTip("Press this button to ez-ssh to container.");
    btn_ssh->setEnabled(true);
  }
}

void DlgEnvironment::addRemoteAccess(const CEnvironment *env, const CHubContainer *cont)
{
  QFont *font = new QFont();
  font->setPointSize(5);

  QPushButton* btn_ssh = new QPushButton("SSH", this);

  ui->cont_remote->addWidget(btn_ssh);
  btn_ssh->setMaximumHeight(18);
  btn_ssh->setFont(*font);
  btn_ssh->setEnabled(false);

  QTimer *timer = new QTimer(this);


  connect(timer, &QTimer::timeout, [btn_ssh, env, cont, this](){
    this->check_status(btn_ssh, env, cont);
  });
  check_status(btn_ssh, env, cont);
  timer->start(10000);

  connect(ui->btn_ssh_all, &QPushButton::clicked, btn_ssh, &QPushButton::click);

  connect(btn_ssh, &QPushButton::clicked, [this, env, cont, btn_ssh](){
    emit this->ssh_to_container_sig(env, cont, (void *)btn_ssh);
  });
}

////////////////////////////////////////////////////////////////////////////

DlgEnvironment::~DlgEnvironment() {
  delete ui;
}

///////////////////////////////////////////////////////////////////////////

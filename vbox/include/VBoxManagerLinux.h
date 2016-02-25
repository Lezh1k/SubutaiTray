#ifndef VBOX_MANAGER_LINUX_H
#define VBOX_MANAGER_LINUX_H

#include <stdint.h>
#include <VBox/com/string.h>

#include "VBoxCommons.h"
#include "VirtualBoxHdr.h"
#include "IVBoxManager.h"

class CVBoxManagerLinux : public IVBoxManager
{
  class CEventListenerLinux : public IEventListener {
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_IEVENTLISTENER
    NS_DECL_OWNINGTHREAD
    CEventListenerLinux(CVBoxManagerLinux* instance);
  private:
    CVBoxManagerLinux* m_instance;
    virtual ~CEventListenerLinux();
  };

 private:
  friend class CVBoxManagerSingleton;

  CVBoxManagerLinux();
  virtual ~CVBoxManagerLinux();

  nsCOMPtr<nsIServiceManager> m_service_manager;
  nsCOMPtr<IVirtualBox> m_virtual_box;
  nsCOMPtr<nsIComponentManager> m_component_manager;
  nsCOMPtr<IEventSource> m_event_source;
  nsCOMPtr<IEventListener> m_event_listener;

  static com::Bstr machine_id_from_machine_event(IEvent* event);

  virtual void on_machine_state_changed(IEvent* event);
  virtual void on_machine_registered(IEvent* event);
  virtual void on_session_state_changed(IEvent* event);
  virtual void on_machine_event(IEvent* event);

public:
  virtual int init_machines(void);
  virtual int launch_vm(const com::Bstr& vm_id, vb_launch_mode_t lm = VBML_HEADLESS);
  virtual int turn_off(const com::Bstr& vm_id, bool save_state = false);

};

#endif //VBOX_MANAGER_LINUX_H
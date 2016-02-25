#include "VirtualMachineWin.h"
#include <stdint.h>
#include <assert.h>

CVirtualMachineWin::CVirtualMachineWin(IMachine *com_machine) {
  assert(com_machine != NULL);
  m_internal_machine = com_machine;  
  m_internal_machine->get_Name(m_name.asOutParam());
  m_internal_machine->get_Id(m_iid.asOutParam());
  MachineState state;
  m_internal_machine->get_State(&state);
  set_state((uint32_t)state);

  CoCreateInstance(CLSID_Session,
                   NULL,                 /* no aggregation */
                   CLSCTX_INPROC_SERVER, /* the object lives in a server process on this machine */
                   IID_ISession,         /* IID of the interface */
                   (void**)&m_session);
}
////////////////////////////////////////////////////////////////////////////

CVirtualMachineWin::~CVirtualMachineWin() {  
//  m_internal_machine->Release();
//  m_internal_machine = NULL;
//  m_session->Release();
//  m_session = NULL;
}
////////////////////////////////////////////////////////////////////////////

nsresult CVirtualMachineWin::launch_vm(vb_launch_mode_t mode,
                                       IProgress **progress) {
  BSTR str_mode = SysAllocString(CCommons::VM_lauch_mode_to_wstr(mode));

  m_session->UnlockMachine();
  nsresult rc = m_internal_machine->LaunchVMProcess(m_session,
                                                    str_mode,
                                                    NULL,
                                                    progress);

  SysFreeString(str_mode);
  return rc;
}
////////////////////////////////////////////////////////////////////////////

nsresult CVirtualMachineWin::save_state(IProgress **progress) {
  return m_internal_machine->SaveState(progress);
}
////////////////////////////////////////////////////////////////////////////

nsresult CVirtualMachineWin::turn_off(IProgress **progress) {
  IConsole* console = NULL;
  m_session->UnlockMachine();
  nsresult rc = m_internal_machine->LockMachine(m_session, LockType_Shared);

  if (FAILED(rc)) return rc;

  rc = m_session->get_Console(&console);
  if (FAILED(rc)) return rc;

  rc = console->PowerDown(progress);
  console->Release();
  m_session->UnlockMachine();

  return rc;
}

nsresult CVirtualMachineWin::run_process(const char *path,
                                         const char *user,
                                         const char *password,
                                         int argc,
                                         const char **argv)
{
  return 0;
}
////////////////////////////////////////////////////////////////////////////
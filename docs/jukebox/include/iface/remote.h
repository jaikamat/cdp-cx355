// Display Interface (DI) Main

#ifndef DISPLAY_INTERFACE_REMOTE_H
#define DISPLAY_INTERFACE_REMOTE_H

class JBInterfaceRemote : public InterfaceBase {

  public:
    JBInterfaceRemote(void) : InterfaceBase(NULL) { }
    JBInterfaceRemote(ControlBase *ptr) : InterfaceBase(ptr) { }
    virtual ~JBInterfaceRemote(void) { }

    int Setup(void);
    int ProcessInput(const char *);
    int Refresh(int);

}; /* class JBInterfaceRemote */

#endif // DISPLAY_INTERFACE_REMOTE_H

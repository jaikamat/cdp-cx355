// Editor (DI)

#ifndef DISPLAY_INTERFACE_EDITOR_H
#define DISPLAY_INTERFACE_EDITOR_H

class JBInterfaceEditor : public InterfaceBase {
  public:
    JBInterfaceEditor(void) : InterfaceBase(NULL) { }
    JBInterfaceEditor(ControlBase *ptr) : InterfaceBase(ptr) { }
    virtual ~JBInterfaceEditor(void) { }

    int Setup(void);
    int ProcessInput(const char *);
    int ProcessEvents(unsigned long);
    int Refresh(int);

}; /* class JBInterfaceEditor */

#endif // DISPLAY_INTERFACE_EDITOR_H

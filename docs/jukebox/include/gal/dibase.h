// Graphical ANSI Library (GAL)
// Display Interface (DI) Header

#ifndef DISPLAY_INTERFACE_H
#define DISPLAY_INTERFACE_H

#define NEXT      1
#define PREV      2

class ControlBase;

#include <gal/awindow.h>
#include <ds/linkedlist.h>

class InterfaceBase {
  private:
    LinkedList<AWindow *> *windows;
    int active;

  protected:
    ControlBase *parent;

    int NextWindow(int);
    int PrevWindow(int);
    int NextActive(int);
    int CurrentWindow(void);

  public:
    InterfaceBase(ControlBase *ptr) { parent = ptr;  active = 0;
                  windows = new LinkedList<AWindow *>; }
    virtual ~InterfaceBase(void) { delete windows; }

    void SetParent(ControlBase *ptr) { parent = ptr; } 
    ControlBase *Parent(void) { return parent; }

    virtual int Setup(void);
    virtual int Shutdown(void);
    virtual int ProcessEvents(unsigned long);
    virtual int ProcessInput(const char *);
    virtual int Refresh(int);

    int AddWindow(AWindow *);
    int RemoveWindow(int);

}; /* class InterfaceBase */

#endif // DISPLAY_INTERFACE_H

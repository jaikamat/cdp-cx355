// Display Interface (DI) Startup

#ifndef DISPLAY_INTERFACE_STARTUP_H
#define DISPLAY_INTERFACE_STARTUP_H

class JBInterfaceStartup : public InterfaceBase {

  private:
//    CDDatabase *cddb;
    CDPlayer *cd_player;
    Library *cd_library;

  public:
    JBInterfaceStartup(void) : InterfaceBase(NULL) { }
    JBInterfaceStartup(ControlBase *ptr) : InterfaceBase(ptr) { }
    virtual ~JBInterfaceStartup(void) { }

    int Setup(void);
    int ProcessEvents(unsigned long);

}; /* class JBInterfaceJukebox */

#endif // DISPLAY_INTERFACE_JUKEBOX_H

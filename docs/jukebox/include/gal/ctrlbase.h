// Graphical ANSI Library (GAL)
// Input Control Header

#ifndef CONTROL_H
#define CONTROL_H

#define EXIT		0
#define PROCESSING	1
#define IDLE		2

#define JIFFIES_PER_SEC	20

#include <termio.h>
#include <string.h>

class InterfaceBase;
// class CDDatabase;
class CDPlayer;
class Library;

class ControlBase {
  private:
    InterfaceBase *interface_ptr;

    struct termio orig, raw;
    fd_set input;
    unsigned long jiffie;
    int state;

    void ProcessInput(void);

  protected:
    virtual int ProcessEvents(unsigned long) { return 1; }

  public:
    ControlBase(void);
    virtual ~ControlBase(void);

    virtual int Setup(void);
    virtual int Shutdown(void);

    // Custom virtual functions required for CD Jukebox only
//    virtual CDDatabase *QueryCDDB(void) { return NULL; }
    virtual CDPlayer *QueryCDPlayer(void) { return NULL; }
    virtual Library *QueryLibrary(void) { return NULL; }

    virtual int SetInterface(InterfaceBase *);

}; /* class ControlBase() */

#endif /* CONTROL_H */

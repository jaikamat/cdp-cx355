// CD Jukebox Control Program
#ifndef JUKEBOX_CLASS_H
#define JUKEBOX_CLASS_H

#include <protocol/cdplayer.h>
#include <protocol/library.h>
#include <gal/ctrlbase.h>

class JBControl : public ControlBase {
  private:
//    CDDatabase *cd_database;
    CDPlayer *cd_player;
    Library *cd_library;

  protected:
    int ProcessEvents(unsigned long);

  public:
    JBControl(void) : ControlBase() { }
    ~JBControl(void) { }

    int Setup(void);
    int Shutdown(void);

//    CDDatabase *QueryCDDB(void) { return cd_database; }
    CDPlayer *QueryCDPlayer(void) { return cd_player; }
    Library *QueryLibrary(void) { return cd_library; }

}; /* class JBControl() */

#endif // JUKEBOX_CLASS_H

// Display Interface (DI) Main

#ifndef DISPLAY_INTERFACE_JUKEBOX_H
#define DISPLAY_INTERFACE_JUKEBOX_H

class Queue;
class PlayOrder;
class Cut;
class Play;

class JBInterfaceJukebox : public InterfaceBase {

  private:
    Library *lib;
    CDPlayer *cd;
    struct deck *decks[2];
    struct track *tracks[2];
    Queue *queues[2];
    PlayOrder *order;
    Cut *cutbutton;
    Play *pps;

  protected:
    int queue;
    int previous;

  public:
    JBInterfaceJukebox(void) : InterfaceBase(NULL) { }
    JBInterfaceJukebox(ControlBase *ptr) : InterfaceBase(ptr) { }
    virtual ~JBInterfaceJukebox(void) { }

    int Setup(void);
    int ProcessInput(const char *);
    int ProcessEvents(unsigned long);
    int RefreshTitleArtist(int);
    int Refresh(int);

}; /* class JBInterfaceJukebox */

#endif // DISPLAY_INTERFACE_JUKEBOX_H

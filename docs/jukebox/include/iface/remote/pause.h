// Issue Pause Command

#ifndef REMOTE_PAUSE_BUTTON_H
#define REMOTE_PAUSE_BUTTON_H

#include <jukebox.h>

class RemotePause : public AButton {
  private:
    CDPlayer *cd_player;
    int deck;

  protected:
    int Push(void);

  public:
    RemotePause(char *str, CDPlayer *ptr, int d, int x, int y, int w, int h, 
         int fg, int bg, int a, int s) : AButton(str, x, y, w, h, 
         fg, bg, a, s) { cd_player = ptr;  deck = d; }
    ~RemotePause(void) { }
}; /* class Pause */

#endif // REMOTE_PAUSE_BUTTON_H

// Issue Stop Command

#ifndef REMOTE_STOP_BUTTON_H
#define REMOTE_STOP_BUTTON_H

#include <jukebox.h>

class RemoteStop : public AButton {
  private:
    CDPlayer *cd_player;
    int deck;

  protected:
    int Push(void);

  public:
    RemoteStop(char *str, CDPlayer *ptr, int d, int x, int y, int w, int h, 
         int fg, int bg, int a, int s) : AButton(str, x, y, w, h, 
         fg, bg, a, s) { cd_player = ptr;  deck = d; }
    ~RemoteStop(void) { }
}; /* class Stop */

#endif // REMOTE_STOP_BUTTON_H

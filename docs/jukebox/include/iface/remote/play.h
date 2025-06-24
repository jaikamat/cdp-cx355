// Issue Play Command

#ifndef REMOTE_PLAY_BUTTON_H
#define REMOTE_PLAY_BUTTON_H

#include <jukebox.h>

class RemotePlay : public AButton {
  private:
    CDPlayer *cd_player;
    int deck;

  protected:
    int Push(void);

  public:
    RemotePlay(char *str, CDPlayer *ptr, int d, int x, int y, int w, int h, 
         int fg, int bg, int a, int s) : AButton(str, x, y, w, h, 
         fg, bg, a, s) { cd_player = ptr;  deck = d; }
    ~RemotePlay(void) { }
}; /* class Play */

#endif // REMOTE_PLAY_BUTTON_H

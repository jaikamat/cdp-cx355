// Issue Cut Command, stop currently playing song, start next immediately

#ifndef CUT_BUTTON_H
#define CUT_BUTTON_H

#include <jukebox.h>

class Cut : public AButton {
  private:
    CDPlayer *cd_player;
    struct deck *dk[2];

  protected:
    int Push(void);

  public:
    Cut(char *str, CDPlayer *ptr, struct deck *d1, struct deck *d2,
         int x, int y, int w, int h, int fg, int bg, int a, int s) :
         AButton(str, x, y, w, h, fg, bg, a, s) { 
           cd_player = ptr; 
           dk[0] = d1; 
           dk[1] = d2;
           }
    ~Cut(void) { }
}; /* class Cut */

#endif // CUT_BUTTON_H

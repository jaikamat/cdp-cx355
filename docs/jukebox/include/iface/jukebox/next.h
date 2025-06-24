// Issue Next Command

#ifndef NEXT_BUTTON_H
#define NEXT_BUTTON_H

#include <jukebox.h> 

class Next : public AButton {
  private:
    CDPlayer *cd_player;
    int deck;

  protected:
    int Push(void);

  public:
    Next(char *str, CDPlayer *ptr, int d, int x, int y, int w, int h, 
         int fg, int bg, int a, int s) : AButton(str, x, y, w, h, 
         fg, bg, a, s) { cd_player = ptr;  deck = d; }
    ~Next(void) { }
}; /* class Next */

#endif // NEXT_BUTTON_H

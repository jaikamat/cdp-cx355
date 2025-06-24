// Issue Fade Command

#ifndef FADE_BUTTON_H
#define FADE_BUTTON_H

#include <jukebox.h>

class Fade : public AButton {
  private:
    CDPlayer *cd_player;
    AList *list;
    int deck, fade;

  protected:
    int Push(void);

  public:
    Fade(char *str, CDPlayer *p1, AList *p2, int d, int f, int x, int y,
         int w, int h, int fg, int bg, int a, int s) : AButton(str, x, y,
         w, h, fg, bg, a, s) { cd_player = p1;  list = p2;  
         deck = d;  fade = f; }
    ~Fade(void) { }
}; /* class Fade */

#endif // FADE_BUTTON_H

#ifndef AUTOPLAY_BUTTON_H
#define AUTOPLAY_BUTTON_H

#include <jukebox.h>

class AutoPlay : public AButton {
  private:
    int deck, state;
    CDPlayer *cd;

  public:
    AutoPlay(char *str, CDPlayer *cdp, int x, int y, int w, int h,
             int fg, int bg, int a, int s) : AButton(str, x, y, w, h,
             fg, bg, a, s) { cd = cdp; deck = 0; state = 0; }
    ~AutoPlay(void) { }

    int Push(void);

    int SetDeck(int d) { return (deck = d); }
    int QueryDeck(void) { return deck; }
    int SetState(int s) { return (state = s); }
    int QueryState(void) { return state; }

}; /* class AutoPlay */

#endif // AUTOPLAY_BUTTON_H

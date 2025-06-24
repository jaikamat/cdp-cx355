// Issue Play Command

#ifndef PLAY_BUTTON_H
#define PLAY_BUTTON_H

#include <jukebox.h>
#include <iface/jukebox/pps.h>

class Play : public PPS {

  protected:
    int Push(void);

  public:
    Play(char *str, int x, int y, int w, int h, int fg, int bg,
          int a, int s) : PPS(str, this, x, y, w, h, fg, bg, a, s) { }
    ~Play(void) { }

}; /* class Play */

#endif // PLAY_BUTTON_H

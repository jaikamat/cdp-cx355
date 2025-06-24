// Issue Pause Command

#ifndef PAUSE_BUTTON_H
#define PAUSE_BUTTON_H

#include <jukebox.h>
#include <iface/jukebox/pps.h>

class Pause : public PPS {

  protected:
    int Push(void);

  public:
    Pause(char *str, int x, int y, int w, int h, int fg, int bg,
          int a, int s) : PPS(str, this, x, y, w, h, fg, bg, a, s) { }
    ~Pause(void) { }

}; /* class Pause */

#endif // PAUSE_BUTTON_H

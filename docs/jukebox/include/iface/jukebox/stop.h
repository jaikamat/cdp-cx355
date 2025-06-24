// Issue Stop Command

#ifndef STOP_BUTTON_H
#define STOP_BUTTON_H

#include <jukebox.h>
#include <iface/jukebox/pps.h>

class Stop : public PPS {

  protected:
    int Push(void);

  public:
    Stop(char *str, int x, int y, int w, int h, int fg, int bg,
          int a, int s) : PPS(str, this, x, y, w, h, fg, bg, a, s) { }
    ~Stop(void) { }

}; /* class Stop */

#endif // STOP_BUTTON_H

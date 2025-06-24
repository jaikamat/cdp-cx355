// Issue Play/Pause/Stop Base Object

#ifndef PPS_BASE_BUTTON_H
#define PPS_BASE_BUTTON_H

#include <jukebox.h>
class Play;
class Pause;
class Stop;

class PPS : public AButton {
  private:
    static int state;
    static Play *play;
    static Pause *pause;
    static Stop *stop;

  protected:
    int Push(void);

  public:
    PPS(char *str, void *ptr, int x, int y, int w, int h, int fg, int bg,
          int a, int s) : AButton(str, x, y, w, h, fg, bg, a, s) {
          if (!strcmp(str, "Play"))   play = (Play *)ptr;
          if (!strcmp(str, "Pause"))  pause = (Pause *)ptr;
          if (!strcmp(str, "Stop"))   stop = (Stop *)ptr; }
    ~PPS(void) { }

    int QueryState(void) { return state; }
    int SetState(int s) { return (state = s); }

}; /* class PPS */

#endif // PPS_BASE_BUTTON_H

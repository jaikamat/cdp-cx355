// Issue CD-DB Request

#ifndef IDENTIFY_BUTTON_H
#define IDENTIFY_BUTTON_H

#include <jukebox.h>

class Identify : public AButton {
  private:
    AEdit *start, *stop;
    AList *method;
    CDPlayer *cd_player;
    int state, deck, disc, max, range, pushed;

  public:
    Identify(char *str, CDPlayer *cdp, AList *m, AEdit *s1, AEdit *s2, int d,
            int x, int y, int w, int h, int fg, int bg, int a, int s) :
            AButton(str, x, y, w, h, fg, bg, a, s) { cd_player = cdp;
            method = m; start = s1; stop = s2; deck = d; state = 0;
            disc = 1, pushed = 0;

              if (strlen(stop -> QueryChar()))
                sscanf(stop -> QueryChar(), "%d", &range);

              max = range;
            }

    ~Identify(void) { }
    int ProcessEvents(unsigned long j) { return AButton::ProcessEvents(j); }

    int Push(void);
    int QueryState(void) { return state; }
    int QueryMax(void) { return max; }
    int QueryDisc(void) { return disc; }
    int QueryRange(void) { return range; }
    int QueryPushed(void) { return pushed; }

    int SetState(int s) { return (state = s); }
    int SetMax(int i) { return (max = i); }
    int SetDisc(int d) { return (disc = d); }
    int SetPushed(int p) { return (pushed = p); }

    int NextDisc(void) { return ++disc; }
}; /* class Identify */

#endif // IDENTIFY_BUTTON_H

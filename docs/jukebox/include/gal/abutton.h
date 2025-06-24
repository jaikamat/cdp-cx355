// Graphical ANSI Library (GAL)
// Generic Button

#ifndef ANSI_BUTTON_BASE_H
#define ANSI_BUTTON_BASE_H

#define CLICK_TIME     2

#include <gal/astatic.h>

class AButton : public AStatic {
  private:
    int click_time, count;

  protected:
    virtual int Push(void);

  public:
    AButton(void) : AStatic("Button") { click_time = CLICK_TIME; 
            count = -1; }
    AButton(char *str, int x, int y, int w, int h) : AStatic(str, 
            x, y, w, h) { click_time = CLICK_TIME; count = -1; }
    AButton(char *str, int x, int y, int w, int h, int fg, int bg, 
            int a, int s) : AStatic(str, x, y, w, h, fg, bg, a, s) {
            click_time = CLICK_TIME; count = -1; }
    ~AButton(void) { }

    virtual int ProcessEvents(unsigned long);
    virtual int ProcessInput(const char *);

    int SetClickTime(int);
    int ClickTime(void);

}; /* class AButton */

#endif // ANSI_EDIT_BASE_H

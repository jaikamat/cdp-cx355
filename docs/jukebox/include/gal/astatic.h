// Graphical ANSI Library (GAL)
// Static Text Box

#ifndef ANSI_STATIC_BASE_H
#define ANSI_STATIC_BASE_H

#include <gal/awindow.h>
#include <ds/string.h>

class AStatic : public AWindow {
  protected:
    String text;

  public:
    AStatic(void) { text = "Static"; }
    AStatic(char *str) : AWindow() { text = str; }
    AStatic(char *str, int x, int y, int w, int h) : 
            AWindow(x, y, w, h) { text = str; }
    AStatic(char *str, int x, int y, int w, int h, int fg, int bg, 
            int a, int s) : AWindow(x, y, w, h, fg, bg, a, s) { 
            text = str; }
    ~AStatic(void) { }

    virtual int Refresh(void);
    void SetText(char *);
    char *QueryChar(void);
    String QueryText(void);

}; /* class AStatic */

#endif // ANSI_STATIC_BASE_H

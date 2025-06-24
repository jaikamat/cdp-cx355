// Graphical ANSI Library (GAL)
// Base ASCII Window Support

#ifndef ANSI_WINDOW_BASE_H
#define ANSI_WINDOW_BASE_H

#include <std/ansi.h>

class InterfaceBase;

class AWindow {
  protected:
    InterfaceBase *parent;

    int x, y;         // Upper Left Corner
    int w, h;         // Width and Height
    int fg, bg;       // Foreground and Background Colors
    int attr;         // Curson Attributes (bold/blink/underscore)
    int skip;         // Skip Window When Tabbing Through
    int active;       // Active Window ID
    int id;           // This Window ID

    int Attr(char *, int);
    int Color(char *, int, int);
    int Loc(char *, int, int);

    int Attr(char *buf) { return Attr(buf, attr); }
    int Color(char *buf) { return Color(buf, fg, bg); }
    int Loc(char *buf) { return Loc(buf, x, y); }

  public:
    AWindow(void) { x = y = 1; w = 79; h = 1; fg = WHITE; bg = BLACK;
            active = -1; id = -2; }
    AWindow(int px, int py, int pw, int ph) { x = px; y = py; w = pw;
            h = ph; fg = WHITE; bg = BLACK; attr |= RESET;
            active = -1; id = -2; }
    AWindow(int px, int py, int pw, int ph, int pfg, int pbg, int pa, 
            int ps) { x = px; y = py; w = pw; h = ph; fg = pfg; 
            bg = pbg; attr = pa | RESET; skip = ps; active = -1; id = -2; }

    virtual ~AWindow(void) { }

    void SetParent(InterfaceBase *ptr) { parent = ptr; }
    InterfaceBase *Parent(void) { return parent; }

    virtual int ProcessEvents(unsigned long) { return 1; }
    virtual int ProcessInput(const char *);
    virtual int Refresh(void);
    virtual int Close(void) { return 1; }

    int X(void) { return x; }
    int Y(void) { return y; }
    int Width(void) { return w; }
    int Height(void) { return h; }     
    int Foreground(void) { return fg; }
    int Background(void) { return bg; }
    int Attributes(void) { return attr; }
    int Skip(void) { return skip; }
    int ActiveWindow(void) { return active; }
    int WindowID(void) { return id; }

    // Changes may not appear until refreshed.
    int SetX(int sx) { return x = sx; }
    int SetY(int sy) { return y = sy; }
    int SetWidth(int sw) { return w = sw; }
    int SetHeight(int sh) { return h = sh; }
    int SetForeground(int sfg) { return fg = sfg; }
    int SetBackground(int sbg) { return bg = sbg; }
    int SetAttributes(int sattr) { return attr = sattr; }
    int SetSkip(int sskip) { return skip = sskip; }
    int SetActiveWindow(int a) { return active = a; }
    int SetWindowID(int i) { return id = i; }

}; /* class AWindow */

#endif // ANSI_WINDOW_BASE_H

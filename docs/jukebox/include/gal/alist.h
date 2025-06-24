// Graphical ANSI Library (GAL)
// List Selection Box

#ifndef ANSI_LIST_BASE_H
#define ANSI_LIST_BASE_H

#define ABOVE        1
#define BELOW        2

#include <ds/linkedlist.h>

class AList : public AWindow {
  private:
    int select, side, line;
    LinkedList<String *> list;

  public:
    AList(void) : AWindow() { select = 0; side = BELOW; line = -1; }
    AList(int x, int y, int w, int h) : AWindow(x, y, w, h) { 
          select = 0; side = BELOW; line = -1; }
    AList(int x, int y, int w, int h, int fg, int bg, int a, int s) :
          AWindow(x, y, w, h, fg, bg, a, s) { select = 0; side = BELOW;
          line = -1; }
    ~AList(void) { }

    int SelectedIndex(void);
    int SelectedLine(void);
    String *Selected(void);
    String *Index(int);
    int ListSize(void);

    int SetSide(int);

    int SetList(LinkedList<String *> *);
    int AddString(char *, int = 0);
    int AddString(String *, int = 0);
    int RemoveString(int);
    void ResetList(void) { select = line = 0;  Refresh(); }

    virtual int Up(void);
    virtual int Down(void);
    virtual int Expand(void);
    virtual int Colapse(void);

    virtual int ProcessInput(const char *);
    virtual int Refresh(void);
    virtual int OnDelete(void);
    virtual int OnSelect(void);
    virtual int Close(void);
}; /* class AList */

#endif // ANSI_LIST_BASE

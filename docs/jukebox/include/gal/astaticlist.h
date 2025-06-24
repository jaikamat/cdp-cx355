// Graphical ANSI Library (GAL)
// Static List Selection Box

#ifndef ANSI_STATIC_LIST_BASE_H
#define ANSI_STATIC_LIST_BASE_H

#include <jukebox.h>

class AStaticList : public AWindow {
  protected:
    int select, line;
    LinkedList<String *> list;

  public:
    AStaticList(void) : AWindow() { select = 0; line = 0; }
    AStaticList(int x, int y, int w, int h) : AWindow(x, y, w, h) {
                select = 0; line = 0; }
    AStaticList(int x, int y, int w, int h, int fg, int bg, int a, int s) :
                AWindow(x, y, w, h, fg, bg, a, s) { select = 0; line = 0; }
    ~AStaticList(void) { }

    int SelectedIndex(void);
    String *Selected(void);
    String *Index(int);
    int ListSize(void);

    int SetList(LinkedList<String *> *);
    int ClearList(void);
    int AddString(char *, int);
    int AddString(String *, int);
    int AddString(int row_fg, int row_bg, char *);
    int AddString(int row_fg, int row_bg, String *);
    int RemoveString(int);

    virtual int Up(void);
    virtual int Down(void);
    virtual int PageUp(void);
    virtual int PageDown(void);
    virtual int Home(void);
    virtual int End(void);
    
    virtual int ProcessInput(const char *);
    virtual int Refresh(void);
    virtual int OnSelect(void);
    virtual int OnDelete(void);

}; /* class AStaticList */

#endif // ANSI_STATIC_LIST_BASE_H

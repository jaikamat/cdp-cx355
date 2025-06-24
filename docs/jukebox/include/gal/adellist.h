// Graphical ANSI Library (GAL)
// List Selection Box (With Delete)

#ifndef ANSI_DEL_LIST_BASE_H
#define ANSI_DEL_LIST_BASE_H

class ADelList : public AList {
  private:

  public:
    ADelList(void) : AList() { }
    ADelList(int x, int y, int w, int h) : AList(x, y, w, h) { }
    ADelList(int x, int y, int w, int h, int fg, int bg, int a, int s) :
          AList(x, y, w, h, fg, bg, a, s) { }
    ~ADelList(void) { }

    virtual int OnDelete(void) { return RemoveString(-1); }      

}; /* class ADelList */

#endif // ANSI_DEL_LIST_BASE_H

// Graphical ANSI Library (GAL)
// Editable List Text Box

#ifndef ANSI_EDIT_LIST_BASE_H
#define ANSI_EDIT_LIST_BASE_H

class AEditList : public AList {
  private:
    int position, mode;  // Modes; 0-Edit, 1-List
    String text;

    int Backspace(void);
    int Insert(void);
    int Delete(void);
    int Enter(void);
    int Left(void);
    int Right(void);

  public:
    AEditList(void) : AList() { mode = 1; position = 0; }
    AEditList(int x, int y, int w, int h) : AList(x, y, w, h) { 
              mode = 1; position = 0; }
    AEditList(int x, int y, int w, int h, int fg, int bg, int a, int s) :
              AList(x, y, w, h, fg, bg, a, s) { mode = 1; position = 0; }
    ~AEditList(void) { }

    virtual int ProcessInput(const char *);
    virtual int Refresh(void);
    virtual int Close(void);

}; /* class AEditList */

#endif // ANSI_EDIT_BASE_H

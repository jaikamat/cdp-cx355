// Graphical ANSI Library (GAL)
// Edit Text Box

#ifndef ANSI_EDIT_BASE_H
#define ANSI_EDIT_BASE_H

class AEdit : public AStatic {
  private:
    int position;

    int Backspace(void);
    int Delete(void);
    int Left(void);
    int Right(void);

  protected:
    virtual int Enter(void) { return 1; }

  public:
    AEdit(void) : AStatic("Edit") { position = 0; }
    AEdit(char *str, int x, int y, int w, int h) : 
          AStatic(str, x, y, w, h) { position = 0; }
    AEdit(char *str, int x, int y, int w, int h, int fg, int bg, int a, 
          int s) : AStatic(str, x, y, w, h, fg, bg, a, s) { position=0; }
    ~AEdit(void) { }

    virtual int ProcessInput(const char *);
    virtual int Refresh(void);
    virtual int Close(void);

}; /* class AEdit */

#endif // ANSI_EDIT_BASE_H

// Graphical ANSI Library (GAL)
// CheckBox

#ifndef ANSI_CHECKBOX_BASE_H
#define ANSI_CHECKBOX_BASE_H

class ACheckBox : public AWindow {
  protected:
    String text;
    int flag;

  public:
    ACheckBox(void) { text = "CheckBox"; flag = 0; }
    ACheckBox(char *str, int pf) : AWindow() { text = str; flag = pf; }
    ACheckBox(char *str, int pf, int x, int y, int w, int h) : 
              AWindow(x, y, w, h) { text = str; flag = pf; }
    ACheckBox(char *str, int pf, int x, int y, int w, int h, int fg,
              int bg, int a, int s) : AWindow(x, y, w, h, fg, bg, a, s)
              { text = str; flag = pf; }
    ~ACheckBox(void) { }

    virtual int ProcessInput(const char *);    
    virtual int Refresh(void);

    void SetText(char *);
    String QueryText(void);

    int SetCheckBox(int);
    int QueryCheckBox(void);

}; /* class ACheckBox */

#endif // ANSI_CHECKBOX_BASE_H

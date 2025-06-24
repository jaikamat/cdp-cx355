// Graphical ANSI Library (GAL)
// CheckBox

#ifndef ANSI_CHECKBOX_BASE
#define ANSI_CHECKBOX_BASE

#include <jukebox.h>

void ACheckBox::SetText(char *str) { text = str; }
String ACheckBox::QueryText(void) { return text; }

int ACheckBox::SetCheckBox(int pf) { 

  flag = pf;
  Refresh();

  return flag;
} /* SetCheckBox() */

int ACheckBox::QueryCheckBox(void) { return flag; }

int ACheckBox::ProcessInput(const char *pending) {

  HOTKEY(ENTER, SetCheckBox(!flag));
  HOTKEY(SPACE, SetCheckBox(!flag));

  return 1;
} /* ProcessInput() */

int ACheckBox::Refresh(void) {
  char attr[40], color[20], loc[10], format[40];

  AWindow::Attr(attr);
  AWindow::Color(color);
  AWindow::Loc(loc);

  sprintf(format, "%%s%%s%%s[%%-1s] %%-%ds", AWindow::w - 4);
  fprintf(stdout, format, attr, color, loc, (flag) ? "X" : " ",
          text.Text());

  return 1;
} /* Refresh() */

#endif // ANSI_CHECKBOX_BASE

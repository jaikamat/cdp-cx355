// Graphical ANSI Library (GAL)
// Base ASCII Window Support

#ifndef ANSI_WINDOW_BASE
#define ANSI_WINDOW_BASE

#include <jukebox.h> 

int AWindow::Attr(char *buf, int pattr) {
  sprintf(buf, "%s%s%s%s%s%s", (pattr & RESET) ? RESET_ASC : "",
          (pattr & BOLD) ? BOLD_ASC : "", (pattr & UNDERSCORE) ? 
          UNDERSCORE_ASC : "", (pattr & BLINK) ? BLINK_ASC : "",
          (pattr & REVERSE) ? REVERSE_ASC : "", (pattr & INVISIBLE) ?
          INVISIBLE_ASC : "");
  return strlen(buf);
} /* Attr() */

int AWindow::Color(char *buf, int pfg, int pbg) {
  sprintf(buf, FG BG, pfg, pbg);
  return strlen(buf);
} /* Color() */

int AWindow::Loc(char *buf, int px, int py) {
  sprintf(buf, LOC, py, px);
  return strlen(buf);
} /* Loc() */

int AWindow::ProcessInput(const char *) { return 1; }
int AWindow::Refresh(void) { return 1; }

#endif // ANSI_WINDOW_BASE

// Graphical ANSI Library (GAL)
// Static Text Box

#ifndef ANSI_STATIC_BASE
#define ANSI_STATIC_BASE

#include <jukebox.h>

int AStatic::Refresh(void) {
  char attr[40], color[20], loc[10], format[10], *line;

  line = (char *)malloc(AWindow::w); 
  AWindow::Attr(attr);
  AWindow::Color(color);

  sprintf(format, "%%s%%-%ds", AWindow::w);
  fprintf(stdout, "%s%s", attr, color);

  if (ActiveWindow() == WindowID())
    fprintf(stdout, REVERSE_ASC);

  for (int i = 0; i < AWindow::h; i++) {
    AWindow::Loc(loc, AWindow::x, AWindow::y + i);
    text.Extract(line, i * AWindow::w, AWindow::w);
    fprintf(stdout, format, loc, (line != NULL) ? line : "");
  }

  if (ActiveWindow() == WindowID())
    fprintf(stdout, REVERSE_ASC);

  AWindow::Loc(loc, AWindow::x + (AWindow::w - 1), AWindow::y);
  fprintf(stdout, "%s", loc);

  if (line != NULL)
    free(line);

  return 1;
} /* Refresh() */

void AStatic::SetText(char *str) { text = str; }
String AStatic::QueryText(void) { return text; }
char *AStatic::QueryChar(void) { return text.Text(); }

#endif // ANSI_STATIC_BASE

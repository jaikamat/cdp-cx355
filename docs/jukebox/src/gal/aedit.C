// Graphical ANSI Library (GAL)
// Edit Text Box

#ifndef ANSI_EDIT_BASE
#define ANSI_EDIT_BASE

#include <jukebox.h>

int AEdit::Backspace(void) {

  if (position <= 0)
    return 0;

  AStatic::text.Delete(position - 1, 1);
  position--;
  Refresh();

  return 1;
} /* Backspace() */

int AEdit::Delete(void) {

  if (position >= AStatic::text.QueryLength())
    return 0;

  AStatic::text.Delete(position, 1);
  Refresh();

  return 1;
} /* Delete() */

int AEdit::Left(void) {

  if (position <= 0)
    return 0;

  position--;
  Refresh();  

  return 1;
} /* Left() */

int AEdit::Right(void) {

  if (position >= AStatic::text.QueryLength())
    return 0;

  if (position >= (AWindow::w - 1))
    return 0;

  position++;
  Refresh();

  return 1;
} /* Right() */

int AEdit::ProcessInput(const char *pending) { 

  if (IS_PRINT(pending)) {

    if (AStatic::text.QueryLength() >= AWindow::w)
      return 0;

    AStatic::text.Insert((char *)pending, position);

    if (position < (AWindow::w - 1))
      position++;

    Refresh();

    return 1;
  } 

  HOTKEY(ENTER, Enter());
  HOTKEY(BACKSPACE, Backspace());
  HOTKEY(DELETE, Delete());
  HOTKEY(LEFT, Left());
  HOTKEY(RIGHT, Right());

  return 1;
} /* ProcessInput() */

int AEdit::Refresh(void) {
  char loc[10];

  AStatic::Refresh();
  AWindow::Loc(loc, AWindow::x + position, AWindow::y);
  fprintf(stdout, loc);

  return 1;
} /* Refresh() */

int AEdit::Close(void) {
  position = 0;
  return 1;
} /* Close() */

#endif // ANSI_EDIT_BASE

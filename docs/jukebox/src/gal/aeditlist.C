// Graphical ANSI Library (GAL)
// Editable List Text Box

#ifndef ANSI_EDIT_LIST_BASE
#define ANSI_EDIT_LIST_BASE

#include <jukebox.h>

int AEditList::Backspace(void) {

  if ((position <= 0) || (mode))
    return 0;

  text.Delete(position - 1, 1);
  position--;
  Refresh();

  return 1;
} /* Backspace() */

int AEditList::Insert(void) {

  if (!mode)
    return 1;

  mode = position = 0;
  text = "";

  return ((Close() == -1) ? -1 : Refresh());
} /* Insert() */

int AEditList::Delete(void) {

  if (mode) {
    AList::RemoveString(-1);
  } else {

    if (position >= text.QueryLength())
      return 0;

    text.Delete(position, 1);
    Refresh();
  }

  return 1;
} /* Delete() */

int AEditList::Enter(void) {
  mode = 1;
  Refresh();

  return Close();
} /* Enter() */

int AEditList::Left(void) {

  if ((position <= 0) || (mode))
    return 0;

  position--;
  Refresh();  

  return 1;
} /* Left() */

int AEditList::Right(void) {

  if ((position >= text.QueryLength()) || (mode))
    return 0;

  if (position >= (AWindow::w - 2))
    return 0;

  position++;
  Refresh();

  return 1;
} /* Right() */

int AEditList::ProcessInput(const char *pending) { 

  HOTKEY(ENTER, Enter()); 
  HOTKEY(DELETE, Delete());

  if (mode) {
    HOTKEY(INSERT, Insert());
  } else {
    if (IS_PRINT(pending)) {

      if (text.QueryLength() >= (AWindow::w - 1))
        return 0;

      text.Insert((char *)pending, position);

      if (position < (AWindow::w - 2))
        position++;

      Refresh();

      return 1;
    }

    HOTKEY(BACKSPACE, Backspace());
    HOTKEY(LEFT, Left());
    HOTKEY(RIGHT, Right());
  }

  return AList::ProcessInput(pending);
} /* ProcessInput() */

int AEditList::Refresh(void) {
  char attr[40], color[20], loc[10], format[40];

  if (mode) {
    AList::Refresh();
  } else {
    AWindow::Attr(attr);
    AWindow::Color(color);
    AWindow::Loc(loc);

    sprintf(format, "%%s%%s%%s%%-%ds" RESET_ASC FG BG "%%s",
            AWindow::w - 1, BLACK, WHITE);
    fprintf(stdout, format, attr, color, loc, text.Text(), "*");

    AWindow::Loc(loc, AWindow::x + position, AWindow::y);
    fprintf(stdout, loc);
  }

  return 1;
} /* Refresh() */

int AEditList::Close(void) {

  if (text.QueryLength() > 0) {
    AList::AddString(new String(text));
    text = "";
  }

  return AList::Close();
} /* Close() */

#endif // ANSI_EDIT_BASE

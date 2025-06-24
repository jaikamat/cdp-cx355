// Graphical ANSI Library (GAL)
// List Selection Box

#ifndef ANSI_LIST_BASE
#define ANSI_LIST_BASE

#define ABOVE        1
#define BELOW        2

#include <jukebox.h>

int AList::Up(void) {

  if (line == -1)
    if (select > 0)
      select--;
    else
      return 0;
  else
    if (line == 0)
      if (select > 0)
        select--;
      else
        return 0;
    else
      line--;

  return Refresh();
} /* Up() */

int AList::Down(void) {
  int size = list.ListSize();

  if (line == -1)
    if (select < (size - 1))
      select++;
    else
      return 0;
  else
    if (line == (AWindow::h - 1))
      if (select + line < (size - 1))
        select++;
      else
        return 0;
    else
      line++;

  return Refresh();
} /* Down() */

int AList::Expand(void) {

  if (line >= 0)
    return 1;
    
  select = line = 0;

  return Refresh();
} /* Expand() */

int AList::Colapse(void) {

  if (line == -1)
    return 1;

  select += line;

  return (line = -1);
} /* Colapse() */

int AList::SelectedIndex(void) { return select; }
int AList::SelectedLine(void) { return line; }
String *AList::Selected(void) { return list.Index(select); }
String *AList::Index(int index) { return list.Index(index); }
int AList::ListSize(void) { return list.ListSize(); }

int AList::SetSide(int pside) { return (side = pside); }

int AList::SetList(LinkedList<String *> *plist) {

  list = *plist;

  return 1;
} /* SetList() */

int AList::AddString(char *str, int flag) {

  select = 0;

  if (flag)
    list.InsertFront(new String(str));
  else
    list.InsertRear(new String(str));

  Refresh();
  return 1;
} /* AddString() */

int AList::AddString(String *str, int flag) {

  select = 0;

  if (flag)
    list.InsertFront(str);
  else
    list.InsertRear(str);

  Refresh();
  return 1;
} /* AddString() */

// Pass -1 to remove the current string
int AList::RemoveString(int index) {

  if (index == -1) {
    index = select;

    if (line > 0)
      index += line;
  }

  if ((list.Reset(index) == -1) || (!list.ListSize()))
    return 0;

  if ((select == index) && (select > 0))
    select--;

  delete list.DeleteAt();
  Refresh();

  return 1;
} /* RemoveString() */

int AList::ProcessInput(const char *pending) { 

  HOTKEY(UP, Up());
  HOTKEY(DOWN, Down());
  HOTKEY(PLUS, Expand());
  HOTKEY(MINUS, Colapse());
  HOTKEY(ENTER, OnSelect());
  HOTKEY(DELETE, OnDelete());

  return 1;
} /* ProcessInput() */

int AList::Refresh(void) {
  char attr[40], color[20], loc[10], format[40];
  int i;

  AWindow::Attr(attr);
  AWindow::Color(color);

  sprintf(format, "%%s%%s%%s%%s%%-%ds" RESET_ASC FG BG "%%c", 
          AWindow::w - 1, BLACK, WHITE);

  if (line >= 0) {
    if (side == ABOVE) {
      for (i = -(AWindow::h - 1); i <= 0; i++) {
        AWindow::Loc(loc, AWindow::x, AWindow::y + i);

        fprintf(stdout, format, attr, color, loc, (line == (AWindow::h -
                1 + i)) ? REVERSE_ASC : "", ((select +  AWindow::h + i -
                1) < list.ListSize()) ? (list.Index(select + AWindow::h + 
                i - 1)) -> Text() : "", !i ? 45 : ((((i == -(AWindow::h -
                1)) && (select > 0)) || ((i == -1) && (select +
                AWindow::h < list.ListSize()))) ? 254 : 32));
      }
    } else {
      for (i = 0; i < AWindow::h; i++) {
        AWindow::Loc(loc, AWindow::x, AWindow::y + i);

        fprintf(stdout, format, attr, color, loc, (line == i) ?
                REVERSE_ASC : "", ((select + i) < list.ListSize()) ? 
                (list.Index(select + i)) -> Text() : "", !i ? 45 : 
                ((((i == 1) && (select > 0)) || ((i == (AWindow::h - 
                1)) && (select + AWindow::h < list.ListSize()))) ? 
                254 : 32));
      }
    }
  } else {
    if (ActiveWindow() == WindowID()) {
      AWindow::Loc(loc);
      fprintf(stdout, format, attr, color, loc, REVERSE_ASC,
              (list.ListSize() > 0) ? ((list.Index(select)) -> Text()) :
              "", 43);
    } else {
      AWindow::Loc(loc);
      fprintf(stdout, format, attr, color, loc, "", (list.ListSize() > 0) ?
              ((list.Index(select)) -> Text()) : "", 43);
    }
  }

  AWindow::Loc(loc, AWindow::x + AWindow::w - 1, AWindow::y);
  fprintf(stdout, "%s", loc);

  return 1;
} /* Refresh() */

int AList::OnSelect(void) {
  return Colapse();
} /* OnSelect() */

int AList::OnDelete(void) {
  return 1;
} /* OnDelete() */

int AList::Close(void) {
  return Colapse();
} /* Close() */

#endif // ANSI_LIST_BASE

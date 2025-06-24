// Graphical ANSI Library (GAL)
// Static List Selection Box

#ifndef ANSI_STATIC_LIST_BASE
#define ANSI_STATIC_LIST_BASE

#include <jukebox.h>

int AStaticList::Up(void) {

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

int AStaticList::Down(void) {
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

int AStaticList::PageUp(void) {

  int windowsize=(AWindow::Height());

  for (int i=1; i < windowsize; i++)
    if (Up()==0)
      return 0;

  return Refresh();
} /* PageUp() */

int AStaticList::PageDown(void) {

  int windowsize=(AWindow::Height());

  for (int i=1; i < windowsize; i++)
    if (Down()==0)
      return 0;

  return Refresh();
} /* PageDown() */

int AStaticList::Home(void) {
  line = select = 0;
  return Refresh();
} /* Home() */

int AStaticList::End(void) {
  line = AWindow::h - 1;
  select = list.ListSize() - 1;
  return Refresh();
} /* End() */

int AStaticList::SelectedIndex(void) { return (select + line); }
String *AStaticList::Selected(void) { return list.Index(select + line); }
String *AStaticList::Index(int index) { return list.Index(index); }
int AStaticList::ListSize(void) { return list.ListSize(); }

int AStaticList::SetList(LinkedList<String *> *plist) {

  list = *plist;

  return 1;
} /* SetList() */

int AStaticList::ClearList(void) {

  list.ClearList();

  return 1;
} /* ClearList() */

int AStaticList::AddString(char *str, int flag) {
  select = 0;

  if (!flag)
    list.InsertRear(new String(str));
  else
    list.InsertFront(new String(str));

  Refresh();
  return 1;
} /* AddString() */

int AStaticList::AddString(String *str, int flag) {
  select = 0;

  if (!flag)
    list.InsertRear(str);
  else
    list.InsertFront(str);

  Refresh();
  return 1;
} /* AddString() */

int AStaticList::AddString(int row_fg, int row_bg, char *str) {
  String *ptr = new String(str);

  select = 0;
  ptr->SetColor(row_fg, row_bg);
  list.InsertRear(ptr);

  return 1;
} /* AddString() */

int AStaticList::AddString(int row_fg, int row_bg, String *str) {

  select = 0;
  str->SetColor(row_fg, row_bg);
  list.InsertRear(str);
  Refresh();

  return 1;
} /* AddString() */

// Pass -1 to remove the current string
int AStaticList::RemoveString(int index) {

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

int AStaticList::ProcessInput(const char *pending) {

  HOTKEY(UP, Up());
  HOTKEY(KEY_APOS, Up());
  HOTKEY(DOWN, Down());
  HOTKEY(KEY_FS, Down());
  HOTKEY(PAGEUP, PageUp());
  HOTKEY(KEY_SC, PageUp());
  HOTKEY(PAGEDOWN, PageDown());
  HOTKEY(KEY_PERIOD, PageDown());
  HOTKEY(HOME, Home());
  HOTKEY(END, End());
  HOTKEY(ENTER, OnSelect());
  HOTKEY(DELETE, OnDelete());

  return 1;
} /* ProcessInput() */

int AStaticList::Refresh(void) {
  char attr[40], color[20], loc[10], format[40];
  int i;

  AWindow::Attr(attr);
  AWindow::Color(color);

  if (line >= 0) {
    for (i = 0; i < AWindow::h; i++) {
      AWindow::Loc(loc, AWindow::x, AWindow::y + i);

      sprintf(format, SAVE_LOC FG BG "%%s%%-%ds" RESET_ASC FG BG
        "%%c" LOAD_LOC, list.Index(select)->QueryForeground(),
	list.Index(select)->QueryBackground(), AWindow::w - 1, BLACK, WHITE);

      fprintf(stdout,
              format,  // starting draw location + data string
              loc,     // starting draw location
              (line == i) ? REVERSE_ASC : "", // invert scroll bar?
              ((select + i) < list.ListSize()) ?
              (list.Index(select + i)) -> Text() : "",
              !i ? 45 : (( ((i == 1) && (select > 0)) ||
              ( (i == (AWindow::h - 1)) &&
              (select + AWindow::h < list.ListSize()) ) ) ? 254 : 32));
    }
  } else {
    AWindow::Loc(loc);

    sprintf(format, SAVE_LOC FG BG "%%s%%-%ds" RESET_ASC FG BG
      "%%c" LOAD_LOC, list.Index(select)->QueryForeground(),
      list.Index(select)->QueryBackground(), AWindow::w - 1, BLACK, WHITE);

    fprintf(stdout, format, loc, (list.ListSize() > 0) ?
            ((list.Index(select)) -> Text()) : "", 43);
  }

  AWindow::Loc(loc, AWindow::x /* + AWindow::w - 1 */, AWindow::y);
  fprintf(stdout, "%s", loc);

  return 1;
} /* Refresh() */

int AStaticList::OnSelect(void) {
  return 1;
} /* OnSelect() */

int AStaticList::OnDelete(void) {
  return 1;
} /* OnDelete() */

#endif // ANSI_LIST_BASE

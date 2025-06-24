// Graphical ANSI Library (GAL)
// Generic Button

#ifndef ANSI_BUTTON_BASE
#define ANSI_BUTTON_BASE

#define CLICK_TIME     2

#include <jukebox.h>

int AButton::Push(void) {

  if (count == -1) {
    count = click_time;
    AWindow::attr = BIT_FLIP(attr, REVERSE);
    Refresh();
  }

  return 1;
} /* Push() */

int AButton::ProcessEvents(unsigned long) {

  if (!count) {
    fprintf(stdout, SAVE_LOC);
    AWindow::attr = BIT_FLIP(attr, REVERSE);
    Refresh();
    fprintf(stdout, LOAD_LOC);
  }

  if (count >= 0)
    count--;

  return 1;
} /* ProcessEvents() */

int AButton::ProcessInput(const char *pending) { 

  HOTKEY(ENTER, Push());

  return 1;
} /* ProcessInput() */

int AButton::SetClickTime(int time) { return (click_time = time); }
int AButton::ClickTime(void) { return click_time; }

#endif // ANSI_EDIT_BASE

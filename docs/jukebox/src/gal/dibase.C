// Graphical ANSI Library (GAL)
// Display Interface (DI) Base

#ifndef DISPLAY_INTERFACE_BASE
#define DISPLAY_INTERFACE_BASE

#include <stdarg.h>
#include <string.h>

#include "gal/dibase.h"
#include "gal/ctrlbase.h"
#include "std/ansi.h"

int InterfaceBase::NextWindow(int index) {
  int pos, size = windows -> ListSize();

  for (pos = index + 1; pos != index; pos++) {

    if (pos >= size)
      pos = 0;

    if (!(windows -> Index(pos)) -> Skip())
      break;
  }

  return pos;
} /* NextWindow() */

int InterfaceBase::PrevWindow(int index) {
  int pos, size = windows -> ListSize();

  for (pos = index - 1; pos != index; pos--) {

    if (pos < 0)
      pos = size - 1;

    if (!(windows -> Index(pos)) -> Skip())
      break;
  }

  return pos;
} /* PrevWindow() */

int InterfaceBase::NextActive(int dir) {
  int prev = active;

  // if it returns -1 refresh the entire display.
  if ((active >= 0) && (active < windows -> ListSize()))
    if (((windows -> Index(prev = active)) -> Close()) == -1)
      Refresh(-1);

  if (active >= 0) {
    if (dir == NEXT)
      active = NextWindow(active);
    else
      if (dir == PREV)
        active = PrevWindow(active);
      else
        return -1;

    (windows -> Index(prev)) -> SetActiveWindow(active);
    (windows -> Index(active)) -> SetActiveWindow(active);
    Refresh(prev);
    Refresh(active);
  }

  return active;
} /* NextActive() */

int InterfaceBase::CurrentWindow(void) {

  if ((active >= 0) && (active < windows -> ListSize()))
    return active;

  return -1;
} /* CurrentWindow() */

int InterfaceBase::Setup(void) {

  // Locate the first available window (non-skipable)
  if (windows -> ListSize())
    active = NextWindow(windows -> ListSize());
  else
    active = -1;

  if (active >= 0)
    (windows -> Index(active)) -> SetActiveWindow(active);

  Refresh(-1);

  return 1;
} /* Setup() */

int InterfaceBase::Shutdown(void) {
  parent -> Shutdown();

  return 1;
} /* Shutdown() */

int InterfaceBase::ProcessEvents(unsigned long jiffie) {
  int window, size = windows -> ListSize();

  // Call events within windows
  for (window = 0; window < size; window++)
    if (windows -> Index(window) != NULL)
      (windows -> Index(window)) -> ProcessEvents(jiffie);    

  return 1;
} /* ProcessEvents() */

int InterfaceBase::ProcessInput(const char *pending) {

  // Check for global interface hotkeys
  HOTKEY(ESCAPE, Shutdown());
  HOTKEY(TAB, NextActive(NEXT));
  HOTKEY(ALT_TAB, NextActive(PREV));
  HOTKEY(KEY_REV_APOS, NextActive(PREV));

  // Check for extra commands within active window, 
  // if it returns -1 refresh the entire display.
  if ((active > -1) && (active < windows -> ListSize()))
    if (((windows -> Index(active)) -> ProcessInput(pending)) == -1)
      Refresh(-1);

  return 1;
} /* ProcessInput() */

int InterfaceBase::Refresh(int id) {
  int size = windows -> ListSize();

  if (id < 0) {
    for (int i = 0; i < size; i++) {

      if (windows -> Index(i) == NULL)
        continue;

      (windows -> Index(i)) -> Refresh();
    }

    if (active >= 0)
      (windows -> Index(active)) -> Refresh();

  } else {
    if (id < size)
      (windows -> Index(id)) -> Refresh();
    else
      return 0;
  }

  return 1;
} /* Refresh() */

int InterfaceBase::AddWindow(AWindow *wnd) {
  int id;

  wnd -> SetParent(this);
  windows -> InsertRear(wnd);
  id = windows -> ListSize() - 1;
  (windows -> Index(id)) -> SetWindowID(id);

  return id;
} /* AddWindow() */

int InterfaceBase::RemoveWindow(int id) {
  windows -> Reset(id);
  if (windows -> DeleteAt() != NULL) free(windows -> DeleteAt());
  return 1;
} /* RemoveWindow() */

#endif // DISPLAY_INTERFACE_BASE

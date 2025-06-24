// Issue Pause Command

#ifndef PAUSE_BUTTON
#define PAUSE_BUTTON

#include <iface/jukebox/pause.h>

int Pause::Push(void) {
  SetState(2);

  return PPS::Push();
} /* Push() */

#endif // PAUSE_BUTTON

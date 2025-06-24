// Issue Stop Command

#ifndef STOP_BUTTON
#define STOP_BUTTON

#include <iface/jukebox/stop.h>

int Stop::Push(void) {
  SetState(0);

  return PPS::Push();
} /* Push() */

#endif // STOP_BUTTON

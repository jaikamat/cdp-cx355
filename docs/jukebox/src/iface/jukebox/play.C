// Issue Play Command

#ifndef PLAY_BUTTON
#define PLAY_BUTTON

#include <iface/jukebox/play.h> 

int Play::Push(void) {
  SetState(1);

  return PPS::Push();
} /* Push() */

#endif // PLAY_BUTTON

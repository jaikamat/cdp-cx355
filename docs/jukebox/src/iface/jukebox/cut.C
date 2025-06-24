// Issue Cut Command, stop currently playing song, start next immediately

#ifndef CUT_BUTTON
#define CUT_BUTTON

#include <iface/jukebox/cut.h>

int Cut::Push(void) {

  // deck A playing, deck B ready
  if ((dk[0] -> state == STATE_PLAYING) && 
      (dk[1] -> state == STATE_PAUSED)) { 
      cd_player -> Stop(0);
      return AButton::Push();
      }

  // deck B playing, deck A ready
  if ((dk[0] -> state == STATE_PAUSED) &&
      (dk[1] -> state == STATE_PLAYING)) {
      cd_player -> Stop(1);
      return AButton::Push();
      }

  // Ambiguous state... bailing out
  return AButton::Push();
} /* Push() */

#endif // CUT_BUTTON

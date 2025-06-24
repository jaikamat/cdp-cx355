#ifndef AUTOPLAY_BUTTON
#define AUTOPLAY_BUTTON

#include <iface/jukebox/autoplay.h>

int AutoPlay::Push(void) {
  int i;

  if ((state = !state)) {
    cd -> Play(deck);
    SetText("  Stop  ");
  } else {

    for (i = 0; i < 2; i++)
      cd -> Stop(i);

    SetText("AutoPlay");
  }

  return AButton::Push();
} /* Push() */

#endif // AUTOPLAY_BUTTON

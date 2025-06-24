// Issue Play/Pause/Stop Base Object

#ifndef PPS_BASE_BUTTON
#define PPS_BASE_BUTTON

#include <iface/jukebox/pps.h>

int PPS::state = 0;
Play *PPS::play = NULL;
Pause *PPS::pause = NULL;
Stop *PPS::stop = NULL;

int PPS::Push(void) {
  return AButton::Push();
} /* Push() */

#endif // PPS_BUTTON

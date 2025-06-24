// Issue Stop Command

#ifndef REMOTE_STOP_BUTTON
#define REMOTE_STOP_BUTTON

#include <iface/remote/stop.h>

int RemoteStop::Push(void) {

  cd_player -> Stop(deck);

  return AButton::Push();
} /* Push() */

#endif // REMOTE_STOP_BUTTON

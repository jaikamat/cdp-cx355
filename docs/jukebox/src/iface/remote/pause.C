// Issue Pause Command

#ifndef REMOTE_PAUSE_BUTTON
#define REMOTE_PAUSE_BUTTON

#include <iface/remote/pause.h>

int RemotePause::Push(void) {

  cd_player -> Pause(deck);

  return AButton::Push();
} /* Push() */

#endif // REMOTE_PAUSE_BUTTON

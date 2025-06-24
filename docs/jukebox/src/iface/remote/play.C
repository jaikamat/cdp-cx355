// Issue Play Command

#ifndef REMOTE_PLAY_BUTTON
#define REMOTE_PLAY_BUTTON

#include <iface/remote/play.h>

int RemotePlay::Push(void) {

  cd_player -> Play(deck);

  return AButton::Push();
} /* Push() */

#endif // REMOTE_PLAY_BUTTON

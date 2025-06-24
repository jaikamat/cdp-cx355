// Issue Fade Command

#ifndef FADE_BUTTON
#define FADE_BUTTON

#include <iface/jukebox/fade.h>

int Fade::Push(void) {
//  String *str;
//  char *buf;
  int delay = DEFAULT_FADE;

//  buf = (char *)malloc(4);
//  memset(buf, 0, 4);
//  str = list -> Selected();
//  memcpy(buf, (char *)str -> Text(), 2);
//  sscanf(buf, "%d", &delay);
//  free(buf);

  switch (fade) {
    case FADEIN:
      cd_player -> FadeIn(deck, delay);
      break;
    case FADEOUT:
      cd_player -> FadeOut(deck, delay);
      break;
    default:
      break;
  }

  return AButton::Push();
} /* Push() */

#endif // PLAY_BUTTON

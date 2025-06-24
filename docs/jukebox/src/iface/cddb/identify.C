// Issue CD-DB Request

#ifndef IDENTIFY_BUTTON
#define IDENTIFY_BUTTON

#include <iface/cddb/identify.h>

int Identify::Push(void) {
  int low = 1, hi;
  struct deck *deck_status;
  char str1[3], str2[3];

  if (!state) {
    deck_status = cd_player -> QueryDeck(deck);
    hi = deck_status -> capacity;
    state = method -> SelectedIndex() + 1;

    if (strlen(start -> QueryChar())) {
      sscanf(start -> QueryChar(), "%d", &low);

      if ((low < 1) || (low > hi) || (low > deck_status -> capacity))
        low = 1;
    }

    if (strlen(stop -> QueryChar())) {
      sscanf(stop -> QueryChar(), "%d", &hi);

      if ((hi < 1) || (hi < low) || (hi > deck_status -> capacity))
        hi = deck_status -> capacity;
    }

    sprintf(str1, "%d", low);
    start -> SetText(str1);
    start -> Refresh();

    sprintf(str2, "%d", hi);
    stop -> SetText(str2);
    stop -> Refresh();

    method -> SetSkip(1);
    start -> SetSkip(1);
    stop -> SetSkip(1);
    SetText("  Stop  ");

    max = hi;
    range = (hi - low) + 1;
    disc = low;
    pushed = 1;

    switch (state) {
      case 1:  cd_player -> Contents(deck);        break;
      case 2:  cd_player -> LoadDisc(deck, disc);  break;
      case 3:  break;
    }
  } else {
    switch (state) {
      case 1:  state = 0; cd_player -> StopJob(deck);  break;
      case 2:  state = 3; cd_player -> StopJob(deck);  break;
      case 3:  state = 0;
    }

    method -> SetSkip(0);
    start -> SetSkip(0);
    stop -> SetSkip(0);
    SetText("Identify");
  }

  return AButton::Push();
} /* Push() */

#endif // IDENTIFY_BUTTON

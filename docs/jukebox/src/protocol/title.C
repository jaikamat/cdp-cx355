#ifndef TITLE_ENTRY_CLASS
#define TITLE_ENTRY_CLASS

#include <stdio.h>
#include <protocol/title.h>

TitleEntry &TitleEntry::operator= (const TitleEntry& a) {

  if (&a != this) {

    if ((artist != NULL) && (a.artist != NULL))
      strncpy(artist, a.artist, 40);

    if ((title != NULL) && (a.title != NULL))
      strncpy(title, a.title, 40);

    deck = a.deck;
    disc = a.disc;
    track = a.track;
    available = a.available;
    offset = a.offset;
  }

  return *this;
} /* operator=() */

int TitleEntry::operator== (const TitleEntry& t) const {
  return strcasecmp(title, t.title) == 0;
} /* operator==() */

int TitleEntry::operator!= (const TitleEntry& t) const {
  return strcasecmp(title, t.title) != 0;
} /* operator!=() */

int TitleEntry::operator< (const TitleEntry& t) const {
  char str1[80], str2[80];

  sprintf(str1, "%s %s", title, artist);
  sprintf(str2, "%s %s", t.title, t.artist);

  return strcasecmp(str1, str2) < 0;
} /* operator<() */

int TitleEntry::operator<= (const TitleEntry& t) const {
  char str1[80], str2[80];

  sprintf(str1, "%s %s", title, artist);
  sprintf(str2, "%s %s", t.title, t.artist);

  return strcasecmp(str1, str2) <= 0;
} /* operator<=() */

int TitleEntry::operator> (const TitleEntry& t) const {
  char str1[80], str2[80];

  sprintf(str1, "%s %s", title, artist);
  sprintf(str2, "%s %s", t.title, t.artist);

  return strcasecmp(str1, str2) > 0;
} /* operator>() */

int TitleEntry::operator>= (const TitleEntry& t) const {
  char str1[80], str2[80];

  sprintf(str1, "%s %s", title, artist);
  sprintf(str2, "%s %s", t.title, t.artist);

  return strcasecmp(str1, str2) >= 0;
} /* operator>=() */

#endif /* TITLE_ENTRY_CLASS */

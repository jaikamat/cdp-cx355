#ifndef ARTIST_ENTRY_CLASS
#define ARTIST_ENTRY_CLASS

#include <stdio.h>
#include <protocol/artist.h>

ArtistEntry &ArtistEntry::operator= (const ArtistEntry& a) {

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

int ArtistEntry::operator== (const ArtistEntry& a) const {
  return strcasecmp(artist, a.artist) == 0;
} /* operator==() */

int ArtistEntry::operator!= (const ArtistEntry& a) const {
  return strcasecmp(artist, a.artist) != 0;
} /* operator!=() */

int ArtistEntry::operator< (const ArtistEntry& a) const {
  char str1[80], str2[80];

  sprintf(str1, "%s %s", artist, title);
  sprintf(str2, "%s %s", a.artist, a.title);

  return strcasecmp(str1, str2) < 0;
} /* operator<() */

int ArtistEntry::operator<= (const ArtistEntry& a) const {
  char str1[80], str2[80];

  sprintf(str1, "%s %s", artist, title);
  sprintf(str2, "%s %s", a.artist, a.title);

  return strcasecmp(str1, str2) <= 0;
} /* operator<=() */

int ArtistEntry::operator> (const ArtistEntry& a) const {
  char str1[80], str2[80];

  sprintf(str1, "%s %s", artist, title);
  sprintf(str2, "%s %s", a.artist, a.title);

  return strcasecmp(str1, str2) > 0;
} /* operator>() */

int ArtistEntry::operator>= (const ArtistEntry& a) const {
  char str1[80], str2[80];

  sprintf(str1, "%s %s", artist, title);
  sprintf(str2, "%s %s", a.artist, a.title);

  return strcasecmp(str1, str2) >= 0;
} /* operator>=() */

#endif /* ARTIST_ENTRY_CLASS */

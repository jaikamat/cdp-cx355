#ifndef DECK_DISC_TRACK_ENTRY_CLASS
#define DECK_DISC_TRACK_ENTRY_CLASS

#include <protocol/ddt.h>

// For comparisons, Disc/Deck/Track info is twiddled in to the lower
// 18 bits resulting in a unique representative ID.

// Bits:  0..6:   7 bits (128); representing the track.
// Bits:  7..15:  9 bits (512); representing the disc
// Bits: 16..17:  2 bits (4);   representing the deck

int DeckDiscTrackEntry::operator== (const DeckDiscTrackEntry& d) const {
  return (((deck << 0x10)   | (disc << 0x07)   | (track)) ==
          ((d.deck << 0x10) | (d.disc << 0x07) | (d.track)));
} /* operator==() */

int DeckDiscTrackEntry::operator!= (const DeckDiscTrackEntry& d) const {
  return (((deck << 0x10)   | (disc << 0x07)   | (track)) !=
          ((d.deck << 0x10) | (d.disc << 0x07) | (d.track)));
} /* operator!=() */

int DeckDiscTrackEntry::operator< (const DeckDiscTrackEntry& d) const {
  return (((deck << 0x10)   | (disc << 0x07)   | (track)) <
          ((d.deck << 0x10) | (d.disc << 0x07) | (d.track)));
} /* operator<() */

int DeckDiscTrackEntry::operator<= (const DeckDiscTrackEntry& d) const {
  return (((deck << 0x10)   | (disc << 0x07)   | (track)) <=
          ((d.deck << 0x10) | (d.disc << 0x07) | (d.track)));
} /* operator<=() */

int DeckDiscTrackEntry::operator> (const DeckDiscTrackEntry& d) const {
  return (((deck << 0x10)   | (disc << 0x07)   | (track)) >
          ((d.deck << 0x10) | (d.disc << 0x07) | (d.track)));
} /* operator>() */

int DeckDiscTrackEntry::operator>= (const DeckDiscTrackEntry& d) const {
  return (((deck << 0x10)   | (disc << 0x07)   | (track)) >=
          ((d.deck << 0x10) | (d.disc << 0x07) | (d.track)));
} /* operator>=() */

#endif /* DECK_DISC_TRACK_ENTRY_CLASS */

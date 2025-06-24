#ifndef DECK_DISC_TRACK_ENTRY_CLASS_H
#define DECK_DISC_TRACK_ENTRY_CLASS_H

#include <string.h>

class DeckDiscTrackEntry {
  private:
    char artist[40], title[40];
    int deck, disc, track, available;
    long offset;

  public:
    DeckDiscTrackEntry(void) { memset(artist, 0, 40); memset(title, 0, 40);
      deck = 0; disc = 1; track = 1; offset = 0; available = 0; }
    DeckDiscTrackEntry(char *a, char *t, int de, int di, int tr,
      long o, int av) { strncpy(artist, a, 40); strncpy(title, t, 40);
      deck = de; disc = di; track = tr; offset = o; available = av; }
    ~DeckDiscTrackEntry(void) { }

    int operator== (const DeckDiscTrackEntry&) const;
    int operator!= (const DeckDiscTrackEntry&) const;
    int operator< (const DeckDiscTrackEntry&) const;
    int operator<= (const DeckDiscTrackEntry&) const;
    int operator> (const DeckDiscTrackEntry&) const;
    int operator>= (const DeckDiscTrackEntry&) const;

    void SetArtist(char *str) { strncpy(artist, str, 40); }
    void SetTitle(char *str) { strncpy(title, str, 40); }
    void SetDeck(int i) { deck = i; }
    void SetDisc(int i) { disc = i; }
    void SetTrack(int i) { track = i; }
    void SetOffset(long l) { offset = l; }
    void SetAvailable(int i) { available = i; }

    char *QueryArtist() { return artist; }
    char *QueryTitle() { return title; }
    int QueryDeck() { return deck; }
    int QueryDisc() { return disc; }
    int QueryTrack() { return track; }
    long QueryOffset() { return offset; }
    int QueryAvailable() { return available; }

}; /* class DeckDiscTrackEntry */

#endif /* DECK_DISC_TRACK_ENTRY_CLASS_H */

#ifndef DECK_H
#define DECK_H

struct content {
  unsigned short status;  // 0-Old; 1-Wrong; 2-Presumed; 3-Verified
  unsigned short slot;    // 0-Unknown; 1-Missing; 2-Present
  int cddb;               // Identifier of disc supposedly here
}; /* struct content */

struct deck {
  int disc;		// Currently loaded disc
  int track;		// Current track
  char playmode;	// Current playmode
  int state;		// Current deck state
  int cddb;		// CDDB identifier
  int disc_length;	// Total disc length
  int total_tracks;	// Total tracks on disc
  int *track_lengths;	// Track lengths, (total_tracks * sizeof(int))
  int track_time;       // Total current track length
  int play_time;	// Time in to track
  int warn_time;	// Warning time, from end of disc
  int capacity;		// Capacity of deck
  char model[12];	// Deck model identifier
  struct content contents[400];  // Current deck contents
}; /* struct deck */

#endif // DECK_H

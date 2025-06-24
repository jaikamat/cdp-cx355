#ifndef TRACK_H
#define TRACK_H

struct track {
  char title[40];        // Song title
  char artist[40];       // Song artist
  char catagory[16];     // Song type
  int bmp_in;            // Beats per minute in
  int bmp_out;           // Beats per minute out
  int intro_time;        // Where do we start the track
  int intro_transition;  // Cut/1/5/10
  int outro_time;        // Where do we end the track
  int outro_transition;  // Cut/1/5/10
  int deck;              // Deck the disc is located in
  int disc;              // Slot the disc in located in
  int trk;               // Track on disc
  int length;            // Total track length
  int script;            // Index to linked script
}; /* struct track */

#endif // TRACK_H

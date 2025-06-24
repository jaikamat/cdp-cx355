#ifndef DISC_H
#define DISC_H

#include "std/track.h"
#include "std/cddb.h"

struct disc {
  struct cddb id;         // CD_DB indentification
  char title[40];         // Album title   
  char artist[40];        // Album artist
  int tracks;             // Number of tracks
  struct track *indexs;   // Track indexs in track lib 
}; /* struct disc */

#endif // DISC_H

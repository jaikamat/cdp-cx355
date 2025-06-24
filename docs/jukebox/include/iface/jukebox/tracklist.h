#ifndef TRACK_LIST_BOX_H
#define TRACK_LIST_BOX_H

#include <jukebox.h>
#include <iface/jukebox/queue.h>

class TrackList : public AStaticList {
  private:
    Library *cd_library;
    CDPlayer *cd_player;
    BSTree<class DeckDiscTrackEntry> *ddt;
    BSTree<class TitleEntry> *titles;
    BSTree<class ArtistEntry> *artists;    
    Queue *queues[2];
    int sorted_by, entries, min, max;

    int FormatEntry(char *, int, int, int, char *, char *);
    int JumpToLetter(char);

  public:
    TrackList(Library *lib, CDPlayer *cd, Queue *q1, Queue *q2,
              int x, int y, int w, int h, int fg, int bg, int a, int s) :
              AStaticList(x, y, w, h, fg, bg, a, s) {
              cd_library = lib; cd_player = cd;
              ddt = cd_library -> QueryDeckDiscTrack();
              titles = cd_library -> QueryTitles();
              artists = cd_library -> QueryArtists();
              queues[0] = q1; queues[1] = q2; entries = 0;
              sorted_by = 1; min = 0; max = (h - 1); }
    ~TrackList(void) { }

    int ProcessInput(const char *);
    int ResetList(void);
    int SetSortBy(int);

    int Up(void);
    int Down(void);
    int PageUp(void);
    int PageDown(void);
    int Home(void);
    int End(void);
    int OnSelect(void);
    int Refresh(void);
}; /* class TrackList */

#endif // TRACK_LIST_BOX_H

#ifndef LIBRARY_CLASS_H
#define LIBRARY_CLASS_H

// Allows you to search a library of tracks by location/title/artist

#include "std/disc.h"
#include "ds/linkedlist.h"
#include "ds/bstree.h"
#include "ddt.h"
#include "artist.h"
#include "title.h"


class Library {
  private:
    BSTree<class DeckDiscTrackEntry> ddt;
    BSTree<class ArtistEntry> artists;
    BSTree<class TitleEntry> titles;
    BSTree<class CDDBEntry> cddbs;
    CDPlayer *cd_player;
    int letters[2][26];
    int library;
    
    int SaveDisc(int, struct disc *);
    struct disc *RetrieveDisc(long, long *);

  public:
    Library(CDPlayer *cdp) { cd_player = cdp; library = -1; }
    Library(void) { cd_player = NULL; library = -1; }
    ~Library(void) { }

    int DiscExists(struct disc *);
    int Setup(CDPlayer *);
    int Setup(void) { return Setup(NULL); }
    int Open(void);
    int Close(void);
    int Purge(void);
    int Pack(void);

    int BuildIndecies(void);
    int BuildCDDBIndex(int = 0);
    int BuildDeckDiscTrackIndex(int = 0);
    int BuildTitleIndex(int = 0);
    int BuildArtistIndex(int = 0);

    int QueryLetter(char, int);

    BSTree<class DeckDiscTrackEntry> *QueryDeckDiscTrack(void) { return &ddt; }
    BSTree<class ArtistEntry> *QueryArtists(void) { return &artists; }
    BSTree<class TitleEntry> *QueryTitles(void) { return &titles; }

    int AddDisc(struct disc *);
    int RemoveDisc(long);

    struct track *QueryTrack(int, int, int);
    LinkedList<struct track *> QueryTrackTitle(char *);
    LinkedList<struct track *> QueryTrackArtist(char *);

}; /* class Library() */

#endif // LIBRARY_CLASS_H

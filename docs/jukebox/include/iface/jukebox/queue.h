#ifndef QUEUE_LIST_BOX_H
#define QUEUE_LIST_BOX_H

#include <jukebox.h>

class Queue : public ADelList {
  private:
    LinkedList<struct track *> tracks;
    CDPlayer *cd_player;
    static int playing;
    int deck, refresh;

  public:
    Queue(CDPlayer *cd, int d, int x, int y, int w, int h,
          int fg, int bg, int a, int s) : ADelList(x, y, w, h, fg, bg,
          a, s) { cd_player = cd; deck = d; refresh = 0; }
    ~Queue(void) { }

    struct track *QueryTrack(int);
    int ListSize(void);

    int AddTrack(struct track *, int = 0);
    int RemoveTrack(int);

    int AddEntry(char *, struct track *, int = 0);
    int AddEntry(String *, struct track *, int = 0);

    int SetDeck(int foo) { return (deck = foo); }
    int QueryDeck(void) { return deck; }

    int SetPlaying(int foo) { return (playing = foo); }
    int QueryPlaying(void) { return playing; }

    int SetRefresh(int foo) { return (refresh = foo); }
    int QueryRefresh(void) { return refresh; }

    int OnDelete(void);
    int OnSelect(void);
    int Colapse(void);
}; /* class Queue */

#endif // QUEUE_LIST_BOX_H

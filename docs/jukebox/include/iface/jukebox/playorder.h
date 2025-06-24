#ifndef PLAY_ORDER_LIST_BOX_H
#define PLAY_ORDER_LIST_BOX_H

#include <jukebox.h>
#include <iface/jukebox/tracklist.h>

class PlayOrder : public AList {
  private:
    TrackList *tracklist;

  public:
    PlayOrder(TrackList *tl, int x, int y, int w, int h, int fg, int bg,
              int a, int s) : AList(x, y, w, h, fg, bg, a, s) {
              tracklist = tl; }
    ~PlayOrder(void) { }

    int OnSelect(void);
}; /* class PlayOrder */

#endif // PLAY_ORDER_LIST_BOX_H

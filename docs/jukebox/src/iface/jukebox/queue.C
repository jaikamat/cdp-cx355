#ifndef QUEUE_LIST_BOX
#define QUEUE_LIST_BOX

#include <iface/jukebox/queue.h>

int Queue::playing = 0;

struct track *Queue::QueryTrack(int index) {

  if ((index < 0) || (index >= ListSize()))
    return NULL;

  return tracks.Index(index);
} /* QueryTrack() */

int Queue::ListSize(void) {
  return tracks.ListSize();
} /* ListSize() */

int Queue::AddTrack(struct track *trk, int flag) {

  if (flag)
    tracks.InsertFront(trk);
  else
    tracks.InsertRear(trk);

  return tracks.ListSize();
} /* AddTrack() */

int Queue::RemoveTrack(int index) {
  struct track *trk;

  if ((index < 0) || (index >= ListSize()))
    return 0;

  tracks.Reset(index);

  if ((trk = tracks.Data()) != NULL)
    free(trk);

  tracks.DeleteAt();
  return 1;
} /* RemoveTrack() */

int Queue::OnDelete(void) {
  struct track *trk;

  // Removing prequeued track from non-playing deck
  if ((playing != deck) && (ListSize() >= 2)) {
    if (SelectedLine() >= 0) {
      if (!(SelectedIndex() + SelectedLine())) {
        trk = QueryTrack(1);
        cd_player -> Pause(deck, trk -> disc, trk -> trk);
        refresh = 1;
      }
    } else {
      if (!SelectedIndex()) {
        trk = QueryTrack(1);
        cd_player -> Pause(deck, trk -> disc, trk -> trk);
        refresh = 1;
      }
    }
  }

  // Deleting the last entry, stop the deck
  if (ListSize() == 1)
    cd_player -> Stop(deck);

  RemoveTrack(SelectedIndex());
  return ADelList::OnDelete();
} /* OnDelete() */

int Queue::OnSelect(void) {
  struct track *trk;
  int index;

  if (SelectedLine() >= 0)
    index = SelectedIndex() + SelectedLine();
  else
    index = SelectedIndex();

  if ((index <= 0) || (index >= tracks.ListSize()))
    return AList::OnSelect();

  AddEntry(Index(index) -> Text(), QueryTrack(index), 1);

  tracks.Reset(index + 1);
  tracks.DeleteAt();
  AList::RemoveString(index + 1);

  char str[80];
  sprintf(str, "INDEX: %d", index);  
  PostError(str);

  // Removing prequeued track from non-playing deck
  if ((playing != deck) && ListSize()) {
    trk = QueryTrack(0);
    cd_player -> Pause(deck, trk -> disc, trk -> trk);
    refresh = 1;
  }

  return AList::OnSelect();
} /* OnSelect() */

int Queue::Colapse(void) {
  ResetList();
  return AList::Colapse();
} /* Colapse() */

int Queue::AddEntry(char *str, struct track *trk, int flag) {
  int size = AddTrack(trk, flag);
  char buf[80];

  sprintf(buf, "Str AddString (size=%d)", size); 
  PostError(buf);
 
  return AList::AddString(str, flag);
} /* AddEntry() */

int Queue::AddEntry(String *str, struct track *trk, int flag) {
  int size = AddTrack(trk, flag);
  char buf[80];

  sprintf(buf, "String AddString (size=%d)", size); 
  PostError(buf);
 
  return AList::AddString(str, flag);
} /* AddEntry() */

#endif // QUEUE_LIST_BOX

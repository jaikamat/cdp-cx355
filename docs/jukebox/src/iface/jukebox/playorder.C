#ifndef PLAY_ORDER_LIST_BOX
#define PLAY_ORDER_LIST_BOX

#include <iface/jukebox/playorder.h>

int PlayOrder::OnSelect(void) {
  int foo = AList::OnSelect();
  tracklist -> SetSortBy(SelectedIndex() + 1);
  tracklist -> ResetList();
  return foo;
} /* PlayOrder() */

#endif // PLAY_ORDER_LIST_BOX

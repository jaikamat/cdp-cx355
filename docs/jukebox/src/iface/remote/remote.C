// Display Interface (DI) Main

#ifndef DISPLAY_INTERFACE_REMOTE
#define DISPLAY_INTERFACE_REMOTE

#include <jukebox.h>
#include <iface/remote/play.h>
#include <iface/remote/pause.h>
#include <iface/remote/stop.h>

int JBInterfaceRemote::Setup(void) {

  AddWindow(new AButton("Play ", 2, 2, 5, 1, WHITE, BLACK, 0, 0));
  AddWindow(new AButton("Pause", 2, 4, 5, 1, WHITE, BLACK, 0, 0));
  AddWindow(new AButton("Stop ", 2, 6, 5, 1, WHITE, BLACK, 0, 0));

  // Footer for hotkeys
  AddWindow(new AStatic(FOOTER, 1, 25, 80, 1, WHITE, BLUE, 0, 1));

  InterfaceBase::Setup();

  return 1;
} /* Setup() */

int JBInterfaceRemote::ProcessInput(const char *pending) {
  InterfaceBase::ProcessInput(pending);

  // Interface specific global hotkeys
  HOTKEY(KEY_F1, parent -> SetInterface(new JBInterfaceJukebox(parent)));
  HOTKEY(KEY_F3, parent -> SetInterface(new JBInterfaceCDDB(parent)));

  return 1;
} /* ProcessInput() */

// Set ID to -1 to refresh entire screen
int JBInterfaceRemote::Refresh(int id) {

  // Fill the screen
  if (id < 0)
    fprintf(stdout, FILL, BLUE);  

  InterfaceBase::Refresh(id);

  return 1;
} /* Refresh() */

#endif /* DISPLAY_INTERFACE_REMOTE */

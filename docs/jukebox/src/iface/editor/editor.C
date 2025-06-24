// Database Editor

#ifndef DISPLAY_INTERFACE_EDITOR
#define DISPLAY_INTERFACE_EDITOR

#include <jukebox.h>

int JBInterfaceEditor::Setup(void) {

  // Footer for hotkeys
  AddWindow(new AStatic(FOOTER, 1, 43, 80, 1, WHITE, BLUE, 0, 1));

  InterfaceBase::Setup();
  return 1;
} /* Setup() */

int JBInterfaceEditor::ProcessInput(const char *pending) {
  InterfaceBase::ProcessInput(pending);

  // Interface specific global hotkeys
  HOTKEY(KEY_F1, parent -> SetInterface(new JBInterfaceJukebox(parent)));
  HOTKEY(KEY_F2, parent -> SetInterface(new JBInterfaceRemote(parent)));

  return 1;
} /* ProcessInput() */

int JBInterfaceEditor::ProcessEvents(unsigned long jiffie) {
  return InterfaceBase::ProcessEvents(jiffie);
} /* ProcessEvents() */

int JBInterfaceEditor::Refresh(int id) {

  if (id < 0)
    fprintf(stdout, FILL, BLUE);  

  InterfaceBase::Refresh(id);

  return 1;
} /* Refresh() */

#endif /* DISPLAY_INTERFACE_EDITOR */

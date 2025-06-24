// Display Interface (DI) Startup

#ifndef DISPLAY_INTERFACE_STARTUP
#define DISPLAY_INTERFACE_STARTUP

#include <jukebox.h>

int JBInterfaceStartup::Setup(void) {
  cd_player = parent -> QueryCDPlayer();
//  cddb = parent -> QueryCDDB();
  cd_library = parent -> QueryLibrary();

  InterfaceBase::Setup();
  return 1;
} /* Setup() */

int JBInterfaceStartup::ProcessEvents(unsigned long jiffie) {
  struct message *msg;
  struct deck *info;
  static int state = 1, started = 0, status;

  // CD-Player AutoDetection
  if (state == 1) {
    fprintf(stdout, "\nIdentifing decks.\n");
    cd_player -> Startup();
    return (state = 2);
  }

  if (state == 2) {
    if ((msg = cd_player -> PeekMessage()) != NULL) {
      msg = cd_player -> QueryMessage();

      // Status, 1111 Both available, 1100 Both unavailable
      if (msg -> id == STARTEDUP) {
        info = cd_player -> QueryDeck(msg -> deck);

        if (msg -> deck >= 0) {
          fprintf(stdout, "  Deck %c Ready, %s %d.\n", msg -> deck + 0x41,
                  info -> model, info -> capacity);
          started |= (msg -> deck + 1) + ((msg -> deck + 1) * 0x04);
        } else {
          fprintf(stdout, "  Deck %c Unavailable.\n", -msg -> deck + 0x41);
          started |= (-msg -> deck + 1) * 0x04;
        }

        if (started >= 12)
          return (state = 7);
      } 

      free(msg);
    }
  }

  // CDDB Login and Catagory Verification

  if (state == 5) {
    fprintf(stdout, "\nVerifying CDDB Server (Not implemented).\n");
//    cddb -> Setup();
    return state++;
  }

  if (state == 6) {
    status = SOCK_UNAVAILABLE;
//    status = cddb -> SocketStatus();
//      fprintf(stdout, "  %s", cddb -> Status());

    if (status == SOCK_UNAVAILABLE) {
      fprintf(stdout, "  CDDB Server Unavailable\n");
      return state++;
    }

    if (status == SOCK_CLOSED) {
      fprintf(stdout, "  CDDB Server Available\n");
      return state++;
    }

    if (status > SOCK_CLOSED) {
      return state;
    }
  }

  if (state == 7)
    parent -> SetInterface(new JBInterfaceJukebox(parent));

  return state;
} /* Setup() */

#endif /* DISPLAY_INTERFACE_STARTUP */

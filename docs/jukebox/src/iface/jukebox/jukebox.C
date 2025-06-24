// Display Interface (DI) Jukebox

#ifndef DISPLAY_INTERFACE_JUKEBOX
#define DISPLAY_INTERFACE_JUKEBOX

// this string is really 
#define PHEADER  BLUE_ASC BOLD_ASC "----------------------------------" \
                 RESET_ASC WHITE_ASC BG_BLUE_ASC " Playlist " \
                 BLUE_ASC BOLD_ASC "----------------------------------"
#define THEADER  BLUE_ASC BOLD_ASC "------------------------------" \
                 RESET_ASC WHITE_ASC BG_BLUE_ASC " Available Songs " \
                 BLUE_ASC BOLD_ASC "-------------------------------"
#define DFOOTER  BLUE_ASC BOLD_ASC "----------------------------------" \
                 "--------------------------------------------"
#define HEADER   " D Dsc Trk  Title                               Artist"

#include <jukebox.h>
#include <iface/jukebox/stop.h>
#include <iface/jukebox/play.h>
#include <iface/jukebox/pause.h>

// Provision for a cleaner next track operation... not implemented yet
// #include <iface/jukebox/next.h>

#include <iface/jukebox/cut.h>

// Provision for a track to track fade routine... not implemented yet
// #include <iface/jukebox/fade.h>

#include <iface/jukebox/tracklist.h>
#include <iface/jukebox/playorder.h>
#include <iface/jukebox/queue.h>


int JBInterfaceJukebox::Setup(void) {

  // Part of the prototyping for cross fade and fade out transitions between
  // tracks.  Not implemented and the change runs deep.  Disabled.
  // AList *xfade, *tfade;

  lib = parent -> QueryLibrary();
  cd = parent -> QueryCDPlayer();
  TrackList *list;
  char deck_A[35] = "Unavailable\0", deck_B[35] = "Unavailable\0";

  for (int i = 0; i < 2; i++) {
    decks[i] = cd -> QueryDeck(i);
    tracks[i] = NULL;
  }

  if (decks[0] -> state != STATE_UNAVAILABLE)
    sprintf(deck_A, GREEN_ASC "%s %d" WHITE_ASC " - ",
      decks[0] -> model, decks[0] -> capacity);

  if (decks[1] -> state != STATE_UNAVAILABLE)
    sprintf(deck_B, RED_ASC "%s %d" WHITE_ASC " - ",
      decks[1] -> model, decks[1] -> capacity);

  // Available Decks (We should grey out those unavailable)
  AddWindow(new AStatic(GREEN_ASC "Deck A" WHITE_ASC ":", 1, 1, 17, 1,
    WHITE, BLUE, 0, 1));
  AddWindow(new AStatic(RED_ASC "Deck B" WHITE_ASC ":", 41, 1, 17, 1,
    WHITE, BLUE, 0, 1));

  AddWindow(new AStatic(deck_A, 9, 1, 35, 1, WHITE, BLUE, 0, 1));
  AddWindow(new AStatic(deck_B, 49, 1, 35, 1, WHITE, BLUE, 0, 1));

  // Queue Lists
  queues[0] = new Queue(cd, DECK_A, 2, 12, 78, 5, GREEN, BLACK, 0, 0);
  queues[0] -> SetSide(ABOVE);

  queues[1] = new Queue(cd, DECK_B, 2, 14, 78, 5, RED, BLACK, 0, 0);
  queues[1] -> SetSide(ABOVE);

  queue = DECK_A;
  previous = 0;

  // Available Track List
  list = new TrackList(lib, cd, queues[0], queues[1],
                       2, 18, 78, 22, WHITE, BLACK, 0, 0);
  list -> ResetList();
  AddWindow(list);

  // Play Order Drop Down Box
  order = new PlayOrder(list, 2, 41, 17, 3, WHITE, BLACK, 0, 0);
  order -> AddString("Artist");
  order -> AddString("Title");
  order -> AddString("Deck/Disc/Track");
  order -> SetSide(BELOW);
  AddWindow(order);  

  // Play/Pause/Stop Buttons
  AddWindow(pps = new Play("Play", 2, 8, 4, 1, WHITE, BLACK, 0, 0));
  AddWindow(new Pause("Pause", 8, 8, 5, 1, WHITE, BLACK, 0, 0));
  AddWindow(new Stop("Stop", 15, 8, 4, 1, WHITE, BLACK, 0, 0));

// Provision for a transition fade control.  Not functional and the
// change runs deep.  Disabled.

//  tfade = new AList(25, 8, 11, 7, WHITE, BLACK, 0, 0);
//  tfade -> AddString(" 1s T-Fade");
//  tfade -> AddString(" 2s T-Fade");
//  tfade -> AddString(" 3s T-Fade");
//  tfade -> AddString(" 5s T-Fade");
//  tfade -> AddString("10s T-Fade");
//  tfade -> AddString("15s T-Fade");
//  tfade -> AddString("30s T-Fade");
//  tfade -> SetSide(BELOW);
//  AddWindow(tfade);

  // Cut control. Aborts current playback and moves to next track in queue
  // by issuing a stop to the playing deck and letting the cdplayer object
  // recover from the interruption in playback.  Not pretty, but it works.
  AddWindow(cutbutton = new Cut("Cut", cd, decks[0], decks[1], 39, 8, 3,
    1, WHITE, BLACK, 0, 0));

// Provision for a cross fade control.  Not functional and the
// change runs deep.  Disabled

//  xfade = new AList(45, 8, 11, 7, WHITE, BLACK, 0, 0);
//  xfade -> AddString(" 1s X-Fade");
//  xfade -> AddString(" 2s X-Fade");
//  xfade -> AddString(" 3s X-Fade");
//  xfade -> AddString(" 5s X-Fade");
//  xfade -> AddString("10s X-Fade");
//  xfade -> AddString("15s X-Fade");
//  xfade -> AddString("30s X-Fade");
//  xfade -> SetSide(BELOW);
//  AddWindow(xfade);  

  // Queue Lists
  AddWindow(queues[0]);
  AddWindow(queues[1]);

  // Footer for deck status
  AddWindow(new AStatic(DFOOTER, 2, 7, 89, 1, WHITE, BLUE, 0, 1));

  // Header for playlist
  AddWindow(new AStatic(PHEADER, 2, 10, 111, 1, WHITE, BLUE, 0, 1));
  AddWindow(new AStatic(HEADER, 2, 11, 79, 1, WHITE, BLUE, 0, 1));

  // Header for Title/Artist Index
  AddWindow(new AStatic(THEADER, 2, 16, 111, 1, WHITE, BLUE, 0, 1));
  AddWindow(new AStatic(HEADER, 2, 17, 79, 1, WHITE, BLUE, 0, 1));

  // Footer for hotkeys
  AddWindow(new AStatic(FOOTER, 1, 43, 80, 1, WHITE, BLUE, 0, 1));

  InterfaceBase::Setup();

  return 1;
} /* Setup() */

int JBInterfaceJukebox::ProcessInput(const char *pending) {
  InterfaceBase::ProcessInput(pending);

  // Interface specific global hotkeys

//  Hotkey for remote interface.  Interface incomplete.  Control disabled.
//  HOTKEY(KEY_F2, parent -> SetInterface(new JBInterfaceRemote(parent)));

  // Hotkey for deck rebuild interface.
  HOTKEY(KEY_F3, parent -> SetInterface(new JBInterfaceCDDB(parent)));

  return 1;
} /* ProcessInput() */

int JBInterfaceJukebox::ProcessEvents(unsigned long jiffie) {
  struct message *msg;
  struct track *trk;
  char statestr[20];

  // Screen updates every other jiffies to cut down on flicker.
  if (!(jiffie % 2)) {

    // Refresh the artist/title if it changes without warning
    if (queues[DECK_A] -> QueryRefresh()) {
      if (tracks[DECK_A] != NULL) {
        free(tracks[DECK_A]);
        tracks[DECK_A] = NULL;
      }

      queues[DECK_A] -> SetRefresh(0);
      RefreshTitleArtist(DECK_A);
    }

    // Refresh the artist/title if it changes without warning
    if (queues[DECK_B] -> QueryRefresh()) {
      if (tracks[DECK_B] != NULL) {
        free(tracks[DECK_B]);
        tracks[DECK_B] = NULL;
      }

      queues[DECK_B] -> SetRefresh(0);
      RefreshTitleArtist(DECK_B);
    }

    fprintf(stdout, SAVE_LOC);

    // Refresh the disc/track and run/remain info
    for (int i = 0; i < 2; i++) {
      if (!i) {
        decks[i] -> track ? sprintf(statestr, ": %-2d ",
          decks[i] -> track) : sprintf(statestr, ": -  ");
        fprintf(stdout, LOC RESET_ASC BG_BLUE_ASC GREEN_ASC "Disc"
          WHITE_ASC " : %-3d " GREEN_ASC "Run   " WHITE_ASC ": %s"
          GREEN_ASC LOC "Track" WHITE_ASC "%s" GREEN_ASC " Remain"
          WHITE_ASC ": %s" GREEN_ASC LOC, 5, (40 * i) + 2,
          decks[i] -> disc, cd -> QueryRunTime(i), 6, (40 * i) + 2,
          statestr, cd -> QueryRemainTime(i), 1, (40 * i) + 25);
      } else {
        decks[i] -> track ? sprintf(statestr, ": %-2d ", 
          decks[i] -> track) : sprintf(statestr, ": -  ");
        fprintf(stdout, LOC RESET_ASC BG_BLUE_ASC RED_ASC "Disc "
          WHITE_ASC ": %-3d " RED_ASC "Run   " WHITE_ASC ": %s "
          RED_ASC LOC "Track" WHITE_ASC "%s" RED_ASC " Remain"
          WHITE_ASC ": %s" RED_ASC LOC, 5, (40 * i) + 2, 
          decks[i] -> disc, cd -> QueryRunTime(i), 6, (40 * i) + 2,
          statestr, cd -> QueryRemainTime(i), 1, (40 * i) + 25);
      }

      // Refresh the deck status
      switch (decks[i] -> state) {
        case STATE_STANDBY:
          sprintf(statestr, "Standby");            break;
        case STATE_PLAYING:           
          sprintf(statestr, "Playing");            break;
        case STATE_PAUSED:            
          sprintf(statestr, "Paused");             break;
        case STATE_STOPPED:
          sprintf(statestr, "Stopped");            break;
        case STATE_FASTFORWARD:
          sprintf(statestr, "Fast Forwarding");    break;
        case STATE_REWIND:
          sprintf(statestr, "Rewinding");          break;
        case STATE_FADINGIN:
          sprintf(statestr, "Fading In");          break;
        case STATE_FADINGOUT:
          sprintf(statestr, "Fading Out");         break;
        case STATE_CUEING:
          sprintf(statestr, "Cueing");             break;
        default: 
          sprintf(statestr, "Unknown State");      break;
      }

      fprintf(stdout, "%-14s", statestr);
    }

    fprintf(stdout, LOAD_LOC);
  }

  // Babysit the Queues; the users mucking with the controls
  if (pps -> QueryState() != previous) {
    previous = pps -> QueryState();

    switch (pps -> QueryState()) {
      default:                       break;
      case 0:  // User issued a queue stop
        cd -> Stop(queue);
        cd -> Stop(!queue);
        break;

      case 1:  // User issued a queue play
        queue = DECK_A;

        if (queues[queue] != NULL) {

          // If deck A queue is empty use deck B queue
          if (!queues[queue] -> ListSize())
            queue = DECK_B;

          if (queues[queue] -> ListSize()) {
            trk = queues[queue] -> QueryTrack(0);
            cd -> Play(queue, trk -> disc, trk -> trk);
          }
        }

        if (queues[!queue] != NULL)
          if ((queues[!queue] -> ListSize()) &&
              (decks[!queue] -> state != STATE_PAUSED)) {
            trk = queues[!queue] -> QueryTrack(0);
            cd -> Pause(!queue, trk -> disc, trk -> trk);
          }

        break;

      case 2:  // User issued a queue pause
         cd -> Pause(queue);
         cd -> Pause(!queue);
         break;
    }
  }

  // Cope with messages the CD Player class thinks are important
  if ((msg = cd -> QueryMessage()) != NULL) {
    switch (msg -> id) {
      default:           break;
      case PLAYING:

        // Purge the playing track from the play queue
        if (queues[queue] != NULL) {
          queues[queue] -> RemoveTrack(0);
          queues[queue] -> RemoveString(0);
        }

      case PAUSE:

        // Free the previous track
        if (tracks[msg -> deck] != NULL) {
          free(tracks[msg -> deck]);
          tracks[msg -> deck] = NULL;
          RefreshTitleArtist(-1);
        }

        // Update the current playing track
        if ((tracks[msg -> deck] = lib -> QueryTrack(msg -> deck,
            msg -> disc, msg -> track)) != NULL) {
          tracks[msg -> deck] -> artist[38] = '\0';
          tracks[msg -> deck] -> title[38] = '\0';
          fprintf(stdout, SAVE_LOC);
          fprintf(stdout, RESET_ASC BG FG LOC "%-38s", BLUE, WHITE,
            2, (40 * msg -> deck) + 2, tracks[msg -> deck] -> artist);
          fprintf(stdout, RESET_ASC BG FG LOC "%-38s", BLUE, WHITE, 
            3, (40 * msg -> deck) + 2, tracks[msg -> deck] -> title);
          fprintf(stdout, LOAD_LOC);
        }

        break;

      case TRACKDONE: 

        // Bounce between decks
        if (queues[!queue] != NULL) {

          // Free the previous track
          if (tracks[msg -> deck] != NULL) {
            free(tracks[msg -> deck]);
            tracks[msg -> deck] = NULL;
            RefreshTitleArtist(-1);
          }

          // There's a track on the other deck, play it!
          if (queues[!queue] -> ListSize()) {
            trk = queues[!queue] -> QueryTrack(0);
            cd -> Play(!queue, trk -> disc, trk -> trk);
            queues[!queue] -> SetPlaying(!queue);

            // Queue up the next track
            if (queues[queue] != NULL)
              if (queues[queue] -> ListSize()) {
                trk = queues[queue] -> QueryTrack(0);
                cd -> Pause(queue, trk -> disc, trk -> trk);
              }

            queue = !queue;
          } else {
            // No pending track for other deck, queue up next on same deck 
            if (queues[queue] -> ListSize()) {
              trk = queues[queue] -> QueryTrack(0);
              cd -> Play(queue, trk -> disc, trk -> trk);
              queues[queue] -> SetPlaying(queue);
            } else {
              // Stop both decks just as a precaution
              cd -> Stop(queue);
              cd -> Stop(!queue);
              pps -> SetState(0);
            }
          }
        }

        break;

      case WARNTIME:
        break;
    }

    if (msg != NULL)
      free(msg);
  }

  return InterfaceBase::ProcessEvents(jiffie);
} /* ProcessEvents() */

int JBInterfaceJukebox::RefreshTitleArtist(int flag) {
  int i, j;

  if (flag == DECK_A) {
    i = j = 0;
  } else {
    if (flag == DECK_B) {
      i = j = 1;
    } else {
      i = 0;
      j = 1;
    }
  }

  fprintf(stdout, SAVE_LOC);  

  for (; i <= j; i++)
    if (tracks[i] != NULL) {
      fprintf(stdout, RESET_ASC BG FG LOC "%-38s", BLUE, WHITE,
              2, (40 * i) + 2, tracks[i] -> artist);
      fprintf(stdout, RESET_ASC BG FG LOC "%-38s", BLUE, WHITE,
              3, (40 * i) + 2, tracks[i] -> title);
    } else {
      fprintf(stdout, RESET_ASC BG FG LOC "%-38s", BLUE, WHITE,
              2, (40 * i) + 2, "----- Artist -----");
      fprintf(stdout, RESET_ASC BG FG LOC "%-38s", BLUE, WHITE,
              3, (40 * i) + 2, "----- Title ------");
    }

  fprintf(stdout, LOAD_LOC);
  return 1;
} /* RefreshTitleArtist() */

// Set ID to -1 to refresh entire screen
int JBInterfaceJukebox::Refresh(int id) {

  // Fill the screen
  if (id < 0) {
    fprintf(stdout, FILL SAVE_LOC BOLD_ASC FG, BLUE, BLUE);  

    for (int i = 1; i < 7; i++)
      fprintf(stdout, LOC "|", i, 40);

    fprintf(stdout, LOAD_LOC);
    RefreshTitleArtist(-1);
  }

  InterfaceBase::Refresh(id);

  return 1;
} /* Refresh() */

#endif /* DISPLAY_INTERFACE_JUKEBOX */

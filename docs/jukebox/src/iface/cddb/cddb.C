// CD-DB (DI)

#ifndef DISPLAY_INTERFACE_CDDB
#define DISPLAY_INTERFACE_CDDB

#define WHITE_MSG   "Unknown/Default"
#define GREEN_MSG   "Matched Existing DB"
#define YELLOW_MSG  "Presumed to Match DB"
#define BLUE_MSG    "Bad Match; Corrected"
#define MAGENTA_MSG "Bad Match; Not in Local DB"
#define RED_MSG     "Bad Match; Not in Remote DB"

#include <jukebox.h>
#include <iface/cddb/identify.h>

int JBInterfaceCDDB::Setup(void) {
  AEdit *startA, *stopA, *startB, *stopB;
  AList *methodA, *methodB;
  struct deck *deck_status;  
  char str1[3], str2[3];

//  cddb = parent -> QueryCDDB();
  cd_player = parent -> QueryCDPlayer();
  library = parent -> QueryLibrary();

  sprintf(status[DECK_A], "%s", "Stopped");
  sprintf(status[DECK_B], "%s", "Stopped");

  deck_status = cd_player -> QueryDeck(DECK_A);
  sprintf(str1, "%d", deck_status -> capacity);
  deck_status -> track = 0;

  deck_status = cd_player -> QueryDeck(DECK_B);
  sprintf(str2, "%d", deck_status -> capacity);

  startA  = new AEdit("1", 43, 3, 3, 1, WHITE, BLACK, 0, 0);
  startB  = new AEdit("1", 43, 4, 3, 1, WHITE, BLACK, 0, 0);
  stopA   = new AEdit(str1, 54, 3, 3, 1, WHITE, BLACK, 0, 0);
  stopB   = new AEdit(str2, 54, 4, 3, 1, WHITE, BLACK, 0, 0);
  methodA = new AList(22, 3, 12, 3, WHITE, BLACK, 0, 0);
  methodB = new AList(22, 4, 12, 3, WHITE, BLACK, 0, 0);

  // Static Text Indentifiers
  AddWindow(new AStatic("Identify:", 2, 2, 9, 1, WHITE, BLUE, 0, 1));
  AddWindow(new AStatic("Deck A:", 4, 3, 7, 1, WHITE, BLUE, 0, 1));
  AddWindow(new AStatic("Deck B:", 4, 4, 7, 1, WHITE, BLUE, 0, 1));
  AddWindow(new AStatic("Start:", 36, 3, 6, 1, WHITE, BLUE, 0, 1));
  AddWindow(new AStatic("Start:", 36, 4, 6, 1, WHITE, BLUE, 0, 1));
  AddWindow(new AStatic("Stop:", 48, 3, 5, 1, WHITE, BLUE, 0, 1));
  AddWindow(new AStatic("Stop:", 48, 4, 5, 1, WHITE, BLUE, 0, 1));

  AddWindow(new AStatic(WHITE_MSG, 3, 39, 36, 1, WHITE, BLACK, 0, 1));
  AddWindow(new AStatic(GREEN_MSG, 3, 40, 36, 1, GREEN, BLACK, 0, 1));
  AddWindow(new AStatic(YELLOW_MSG, 3, 41, 36, 1, YELLOW, BLACK, BOLD, 1));
  AddWindow(new AStatic(BLUE_MSG, 41, 39, 37, 1, BLUE, BLACK, BOLD, 1));
  AddWindow(new AStatic(MAGENTA_MSG, 41, 40, 37, 1, MAGENTA, BLACK, BOLD, 1));
  AddWindow(new AStatic(RED_MSG, 41, 41, 37, 1, RED, BLACK, 0, 1));

  // Deck A Identify Controls
  AddWindow((AWindow *)(buttons[0] = new Identify("Identify", cd_player,
    methodA, startA, stopA, DECK_A, 12, 3, 8, 1, WHITE, BLACK, 0, 0)));

  methodA -> AddString("Scan");
  methodA -> AddString("Rebuild");
  methodA -> AddString("ReIndex");
  methodA -> SetSide(BELOW);
  AddWindow(methodA);

  AddWindow(startA);
  AddWindow(stopA);

  // Deck B Identify Controls
  AddWindow(buttons[1] = new Identify("Identify", cd_player, methodB,
            startB, stopB, DECK_B, 12, 4, 8, 1, WHITE, BLACK, 0, 0));

  methodB -> AddString("Scan");
  methodB -> AddString("Rebuild");
  methodB -> AddString("ReIndex");
  methodB -> SetSide(BELOW);
  AddWindow(methodB);

  AddWindow(startB);
  AddWindow(stopB);

  // Footer for hotkeys
  AddWindow(new AStatic(FOOTER, 1, 43, 80, 1, WHITE, BLUE, 0, 1));

  InterfaceBase::Setup();
  return 1;
} /* Setup() */

int JBInterfaceCDDB::ProcessInput(const char *pending) {
  InterfaceBase::ProcessInput(pending);

  // Interface specific global hotkeys
  HOTKEY(KEY_F1, parent -> SetInterface(new JBInterfaceJukebox(parent)));
  //  F2 is indended to jump to the simple remote control interface... 
  //  which isn't finished and doesn't work.
  //  HOTKEY(KEY_F2, parent -> SetInterface(new JBInterfaceRemote(parent)));

  return 1;
} /* ProcessInput() */

int JBInterfaceCDDB::ProcessEvents(unsigned long jiffie) {
  struct deck *data;
  struct disc *dsc;
  struct message *msg;
  char chr, *ptr, path[80], *str, error_msg[14] = "CD Unavail";
  FILE *import;
  int line = 19;

  // Reindex the decks; (even jiffies DECK_A, odd jiffies DECK_B)
  if ((buttons[jiffie % 2] != NULL) && (library != NULL) &&
      (buttons[jiffie % 2] -> QueryState() == 3)) { 

    for (int i = 0; i <= buttons[jiffie % 2] -> ClickTime(); i++)
      buttons[jiffie % 2] -> ProcessEvents(jiffie);

    library -> BuildCDDBIndex(1);
    fprintf(stdout,LOC RESET_ASC FG BG "Indexing: Dck/Dsc/Trk " CLEAR_EOL,
            (int)(line + (jiffie % 2) * 10), 45, WHITE, BLUE);
    library -> BuildDeckDiscTrackIndex();
    fprintf(stdout,LOC RESET_ASC FG BG "Indexing: Title " CLEAR_EOL,
            (int)(line + (jiffie % 2) * 10), 45, WHITE, BLUE);
    library -> BuildTitleIndex();
    fprintf(stdout,LOC RESET_ASC FG BG "Indexing: Artist " CLEAR_EOL,
            (int)(line + (jiffie % 2) * 10), 45, WHITE, BLUE);
    library -> BuildArtistIndex();

//    fprintf(stdout, LOC RESET_ASC FG BG "Indexing: Keyword " CLEAR_EOL,
//            line + (jiffie % 2) * 10, 45, WHITE, BLUE);
//    library -> BuildKeywordIndex();

    sprintf(status[jiffie % 2], "Complete");
    RefreshTable(jiffie % 2, 1);
    Refresh(CurrentWindow());
    buttons[jiffie % 2] -> Push();
  }

  if ((msg = cd_player -> QueryMessage()) != NULL) {
    switch(msg -> id) {
      default:           break;
      case DISCCDDB:     break;
      case DISCMEMOSET:  break;

      case TOTALTRACKS:
        sprintf(status[msg -> deck], "%s", "Calculating CDDB");
        RefreshTable(msg -> deck, 1);
        break;

      case UPDATEDCONTENTS:
        sprintf(status[msg -> deck], "Updating Contents");
        RefreshTable(msg -> deck, 1);
        buttons[msg -> deck] -> Push();
        break;

      case CUEING:
        sprintf(status[msg -> deck], "%s", "Loading Disc");
        RefreshTable(msg -> deck, 1);
        break;

      case NODISC:

        if (buttons[msg -> deck] -> QueryState() == 2)
          if (buttons[msg -> deck] -> QueryDisc() >=
              buttons[msg -> deck] -> QueryMax()) {
            sprintf(status[msg -> deck], "Deck Rebuild Complete");
            RefreshTable(msg -> deck, 1);
            buttons[msg -> deck] -> Push();
          } else {
            buttons[msg -> deck] -> NextDisc();
            cd_player -> LoadDisc(msg -> deck,
              buttons[msg -> deck] -> QueryDisc());
            sprintf(status[msg -> deck], "No Disc");
          }

        RefreshTable(msg -> deck, 0);
        break;

      case DISCLOADED:
        data = cd_player -> QueryDeck(msg -> deck);

        if (buttons[msg -> deck] -> QueryState() == 2) {
          // We should really eventually ask the library to do the lookup.
          // Import data from local text file

          sprintf(path, "cddb/%.8X", data -> cddb);
          sprintf(status[msg -> deck], "Looking Up %.8X", data -> cddb);

          fprintf(stdout, SAVE_LOC LOC RESET_ASC FG BG "%d Looking Up %.8X "
                 "Locally" CLEAR_EOL LOAD_LOC, 6, 2, WHITE, BLUE,
                 buttons[msg -> deck] -> QueryDisc(), data -> cddb);

          if ((import = fopen(path, "r")) != NULL) {
            fseek(import, 0, SEEK_END);
            str = (char *)calloc(ftell(import), 1);
            fseek(import, 0, SEEK_SET);
            ptr = str;

            while ((chr = fgetc(import)) != EOF) {
              *ptr = chr;
              ptr++;
            }

            fclose(import);
            dsc = Import(str, msg->deck,buttons[msg->deck]->QueryDisc());

            // There are issue with the local CDDB file, unparsable
            if (dsc == NULL) {
              cd_player -> SetContent(msg -> deck, buttons[msg -> deck] ->
                QueryDisc() - 1, -1, SLOT_NOT_LOCAL, 0);
              cd_player -> SetDiscMemo(msg->deck, buttons[msg -> deck] ->
                QueryDisc(), error_msg);
            } else {
              cd_player -> SetDiscMemo(msg->deck, buttons[msg -> deck] ->
                QueryDisc(), dsc->artist);
              library -> AddDisc(dsc);
            }

            if (dsc != NULL)
              free(dsc);

            if (str != NULL)
              free(str);
          } else {
            cd_player -> SetDiscMemo(msg -> deck, buttons[msg -> deck] ->
              QueryDisc(), error_msg);
            cd_player -> SetContent(msg -> deck, buttons[msg -> deck] ->
              QueryDisc() - 1, -1, SLOT_NOT_LOCAL, 0);
          }

          // Move on the next disc or stop
          if (buttons[msg -> deck] -> QueryDisc() >=
              buttons[msg -> deck] -> QueryMax()) {
            // Move on the state 3 to ReIndex
            buttons[msg -> deck] -> SetState(3);
          } else {
            buttons[msg -> deck] -> NextDisc();
            cd_player -> LoadDisc(msg -> deck, 
              buttons[msg -> deck] -> QueryDisc());
          }
        }

        RefreshTable(msg -> deck, 0);
    }

    if (msg != NULL)
      free(msg);
  }

  for (int i = 0; i < 2; i++)
    if (buttons[i] -> QueryPushed()) {
      RefreshTable(i, 1);
      buttons[i] -> SetPushed(0);
    }

  return InterfaceBase::ProcessEvents(jiffie);
} /* ProcessEvents() */

int JBInterfaceCDDB::ColorizeTable(char *str,int slot,int status,long cddb){
  char color[16], chr;

  memset(color, 0, 16);

  switch (slot) {
    default:         
    case SLOT_UNKNOWN:  chr = '.';  break;
    case SLOT_EMPTY:    chr = '-';  break;
    case SLOT_FULL:     chr = 'o';
  }

  switch (status) {
    default:
    case SLOT_UNKNOWN:                                                 break;
    case SLOT_PRESUMED:    sprintf(color, "%s", BOLD_ASC YELLOW_ASC);  break;
    case SLOT_MATCHED:     sprintf(color, "%s", GREEN_ASC);            break;
    case SLOT_CORRECTED:   sprintf(color, "%s", BOLD_ASC BLUE_ASC);    break;
    case SLOT_NOT_LOCAL:   sprintf(color, "%s", BOLD_ASC MAGENTA_ASC); break;
    case SLOT_NOT_REMOTE:  sprintf(color, "%s", RED_ASC);              break;
  }

//  if (!cddb)
//    sprintf(color, "%s", RESET_ASC);

  sprintf(str, "%s%s%c%s", RESET_ASC, color, chr, RESET_ASC);
  return 1;
} /* ColorizeTable() */

void JBInterfaceCDDB::RefreshTable(int table,int flag) {
  struct deck *data;
  int i, disc, range, max, line = 19, row, col, num, percent = 0;
  char str[20];

  data = cd_player -> QueryDeck(table);
  disc = buttons[table] -> QueryDisc();
  range = buttons[table] -> QueryRange();
  max = buttons[table] -> QueryMax();
  num = disc - max + range - 1;

  if (disc == max)
    num = range;

  if (buttons[table] -> QueryState())
    if ((percent = num * 100 / range) < 0)
      percent = 0;
    else
      if (percent > 100)
        percent = 0;

  fprintf(stdout, SAVE_LOC LOC RESET_ASC FG BG "Deck %c: %3d%% "
          "[Disc %3d; %3d/%3d]  Status: %-33s", line + table * 10, 3, WHITE,
          BLUE, 0x41 + table, percent, ((disc <= max) ? disc : max),
          ((num <= range) ? num : range), range, status[table]);

  if (flag) {
    fprintf(stdout, LOC FG BG, line + table * 10, 3, WHITE, BLACK);

    for (i = 0; i < 400; i++) {
      sprintf(str, "%s", " ");

      if (!(i % 50))
        fprintf(stdout, LOC FG BG, ++line + table * 10, 3, WHITE, BLACK);

      if (!(i % 10))
        if ((i + 1) <= data -> capacity)
          fprintf(stdout, " %3d ", i + 1);
        else
          fprintf(stdout, "     ");

      if (i < data -> capacity)
        ColorizeTable(str, data -> contents[i].slot,
                      data -> contents[i].status,
                      data -> contents[i].cddb);

      fprintf(stdout, "%s", str);
    }
  } else {
    disc = ((disc >= 2) ? disc : 2);
    row = (disc - 2) / 50;
    col = (disc - 2) % 50;
    sprintf(str, "%s", " ");

    if ((disc - 1) < data -> capacity)
      ColorizeTable(str, data -> contents[disc - 2].slot,
                    data -> contents[disc - 2].status,
                    data -> contents[disc - 2].cddb);

    fprintf(stdout, LOC RESET_ASC FG BG "%s", line + 1 + table * 10 + row,
            8 + ((col / 10) * 15) + (col % 10), WHITE, BLACK, str);
  }

  fprintf(stdout, LOAD_LOC);
} /* RefreshTable() */

// Set ID to -1 to refresh entire screen
int JBInterfaceCDDB::Refresh(int id) {

  // Fill the screen
  if (id < 0) {
    fprintf(stdout, FILL, BLUE);  
    RefreshTable(DECK_A, 1);
    RefreshTable(DECK_B, 1);
  }

  InterfaceBase::Refresh(id);

  return 1;
} /* Refresh() */

struct disc *JBInterfaceCDDB::Import(char *str, int deck, int disc) {
  struct disc *dsc;
  char buf1[160], buf2[80], *brk, *strt, *nl, *ptr = str, *dbl;
  int index, offset, size = strlen(str), title = 0, tracks = -1;
  int doublekeyflag = 0, dindex = -1;

  do {
    bzero(buf1, 160);
    str += ((char *)memccpy(buf1, str, 10, 80) - buf1);

    if (!strncasecmp(buf1, "TTITLE=DATA", 11))
      continue;

    if (!strncasecmp(buf1, "TTITLE", 6))
      sscanf(buf1, "TTITLE%d=%*s", &tracks);

  } while (str < (ptr + size));

  if (tracks == -1)
    return NULL;

  tracks++;
  str = ptr;

  // Remember the client MUST free the structure once they're done
  dsc = (struct disc *)calloc(sizeof(struct disc), 1);
  dsc -> indexs = (struct track *)calloc(sizeof(struct track)*tracks, 1);
  dsc -> id.offsets = (long *)calloc(sizeof(long) * tracks, 1);

  bzero(dsc -> id.catagory, 16);
  bzero(dsc -> title, 40);
  bzero(dsc -> artist, 40);
  dsc -> id.tracks = tracks;
  dsc -> tracks = tracks;

  do {
    bzero(buf1, 160);
    bzero(buf2, 80);
    dbl = (char *)memccpy(buf1, str, 10, 80);
    if (dbl != NULL) { 
      str += (dbl - buf1);
      } else {
      PostError("NULL pointer in cddb.C Import() first memccpy!");
      break;
      }

    if (doublekeyflag) {
      dbl = (char *)memccpy(buf1, str, 10, 80);
      if (dbl != NULL) { 
        str += (dbl - buf1);
        } else {
        PostError("NULL pointer cddb.C Import doublekey flag memccpy!");
        break;
        }
      doublekeyflag = 0;
      }

    dbl = (char *)memccpy(buf2, str, 10, 80);
    if (dbl == NULL) { 
      bzero(buf2, 80);
      }

    if (!strncasecmp(buf1, "# Track frame offsets", 21)) {
      for (offset = 0; offset < tracks; offset++) {
        str += ((char *)memccpy(buf1, str, 10, 80) - buf1);
        sscanf(buf1, "#       %ld\n", &(dsc -> id.offsets[offset]));
      }

      continue;
    } 

    if (!strncasecmp(buf1, "# Disc length", 13)) {
      sscanf(buf1, "# Disc length: %d", &(dsc -> id.length));
      continue;
    }

    if (!strncasecmp(buf1, "DISCID=", 7)) {
      sscanf(buf1, "DISCID=%8lx", &(dsc -> id.id));
      continue;
    }

    if (!strncasecmp(buf1, "DTITLE=", 7)  && (!title)) {
      strt = (strchr(buf1, '=') + 1);
      if (!strncasecmp(buf2, "DTITLE=", 7)) {
        brk = (strchr(buf2, '=') + 1);
        strncpy(buf1 + strlen(buf1) - 1, brk, 80);
        doublekeyflag = 1;
        }

      while (isspace(*strt)) strt++;

      if ((nl = strchr(strt, '\n')) != NULL)
        bzero(nl, 1);

      if (strchr(strt, '\r') != NULL)
        bzero(strchr(strt, '\r'), 1);

      if ((brk = strchr(strt, '/')) == NULL) {
        if (strncasecmp(strt, "the ", 4)) { 
          strncpy(dsc -> artist, strt, 39);
          strncpy(dsc -> title, strt, 39);
          } else {
            if (strlen(strt + 4) > 33) {
              strncpy(dsc -> artist, strt + 4, 39);
              strncpy(dsc -> title, strt + 4, 39);
              } else {
              strncpy(dsc -> artist, strt + 4, 33);
              strncpy((dsc -> artist) + strlen(dsc -> artist), THE, 6);
              strncpy(dsc -> title, strt + 4, 33);
              strncpy((dsc -> title) + strlen(dsc -> title), THE, 6);
              }
          }       
      } else {
        if (isspace(*(brk - 1))) bzero(brk-1, 2); else bzero(brk,1);
        if (strncasecmp(strt, "the ", 4)) {
          strncpy(dsc -> artist, strt, 39);
          } else {
            if (strlen(strt + 4) > 33) {
              strncpy(dsc -> artist, strt + 4, 39);
              } else {
              strncpy(dsc -> artist, strt + 4, 33);
              strncpy((dsc -> artist) + strlen(dsc -> artist), THE, 6);
              }
          }
        brk++;
        while (isspace(*brk)) brk++;
        if (strncasecmp(brk, "the ", 4)) {
          strncpy(dsc -> title, brk, 39);
          } else {
            if (strlen(brk + 4) > 33) {
              strncpy(dsc -> title, brk + 4, 39);
              } else {
              strncpy(dsc -> title, brk + 4, 33);
              strncpy((dsc -> title) + strlen(dsc -> title), THE, 6);
              }
          }          
        if (!strncasecmp(strt, "Various", 7) ||
            !strncasecmp(strt, "Soundtra", 8) ) {
          if (strncasecmp(brk, "the ", 4)) {
            strncpy(dsc -> artist, brk, 39);
            } else {
              if (strlen(brk + 4) > 33) {
                strncpy(dsc -> artist, brk + 4, 39);
                } else {
                strncpy(dsc -> artist, brk + 4, 33);
                strncpy((dsc -> artist) + strlen(dsc -> artist), THE, 6);
                }
            }
          }
      }

      title = 1;
      continue;
    }

    if (!strncasecmp(buf1, "TTITLE", 6)) {
      dindex = -1; // I'm reseting this to trap an endcase. JRB
      sscanf(buf1, "TTITLE%d=", &index);
      sscanf(buf2, "TTITLE%d=", &dindex);
      strt = (strchr(buf1, '=') + 1);
/* debug code
      fprintf(stdout, SAVE_LOC LOC RESET_ASC FG BG CLEAR_EOL
        LOAD_LOC, 9, 1, WHITE, BLUE);
      fprintf(stdout, SAVE_LOC LOC RESET_ASC FG BG CLEAR_EOL
        LOAD_LOC, 10, 1, WHITE, BLUE);
      fprintf(stdout, SAVE_LOC LOC RESET_ASC FG BG CLEAR_EOL
        LOAD_LOC, 11, 1, WHITE, BLUE);
      fprintf(stdout, SAVE_LOC LOC RESET_ASC FG BG CLEAR_EOL
        LOAD_LOC, 12, 1, WHITE, BLUE);
      fprintf(stdout, SAVE_LOC LOC RESET_ASC FG BG CLEAR_EOL
        LOAD_LOC, 13, 1, WHITE, BLUE);
      fprintf(stdout, SAVE_LOC LOC RESET_ASC FG BG CLEAR_EOL
        LOAD_LOC, 14, 1, WHITE, BLUE);
      fprintf(stdout, SAVE_LOC LOC RESET_ASC FG BG CLEAR_EOL
        LOAD_LOC, 15, 1, WHITE, BLUE);
      fprintf(stdout, SAVE_LOC LOC RESET_ASC FG BG CLEAR_EOL
        "Active track: %d " RESET_ASC FG BG "  Advance Track: %d"
        LOAD_LOC, 9, 1, YELLOW, BLUE, index, GREEN, BLUE, dindex);
      fprintf(stdout, SAVE_LOC LOC RESET_ASC FG BG CLEAR_EOL
        "%s" LOAD_LOC, 10, 1, YELLOW, BLUE, buf1);
      fprintf(stdout, SAVE_LOC LOC RESET_ASC FG BG CLEAR_EOL
        "%s" LOAD_LOC, 11, 1, GREEN, BLUE, buf2);
*/
      if (index == dindex) {
        brk = (strchr(buf2, '=') + 1);
        strncpy(buf1 + strlen(buf1) - 1, brk, 80);
        doublekeyflag = 1;
        }

      while (isspace(*strt)) strt++;

      if (!strncasecmp(strt, "DATA", 4))
        continue;

      if ((nl = strchr(strt, '\n')) != NULL)
        bzero(nl, 1);

      if (strchr(strt, '\r') != NULL)
        bzero(strchr(strt, '\r'), 1);

      dsc -> indexs[index].deck = deck;
      dsc -> indexs[index].disc = disc;
      dsc -> indexs[index].trk = index + 1;

      if ((brk = strchr(strt, '/')) == NULL) {
        strncpy(dsc -> indexs[index].artist, dsc -> artist, 39);
        if (strncasecmp(strt, "the ", 4)) { 
          strncpy(dsc -> indexs[index].title, strt, 39);
          } else {
            if (strlen(strt + 4) > 33) {
              strncpy(dsc -> indexs[index].title, strt + 4, 39);
              } else {
              strncpy(dsc -> indexs[index].title, strt + 4, 33);
              strncpy((dsc -> indexs[index].title) + 
                strlen(dsc -> indexs[index].title) ,THE , 6);
              }
          }

      } else {
        if (isspace(*(brk - 1))) bzero(brk-1, 2); else bzero(brk,1);
        if (strncasecmp(strt, "the ", 4)) {
          strncpy(dsc -> indexs[index].artist, strt, 39);
          } else {
            if (strlen(strt + 4) > 33) {
              strncpy(dsc -> indexs[index].artist, strt + 4, 39);
              } else {
              strncpy(dsc -> indexs[index].artist, strt + 4, 33);
              strncpy((dsc->indexs[index].artist) + 
                strlen(dsc -> indexs[index].artist), THE, 6);
              }
          }
        brk++;
        while (isspace(*brk)) brk++;
        if (strncasecmp(brk, "the ", 4)) {
          strncpy(dsc -> indexs[index].title, brk, 39);
          } else {
            if (strlen(brk + 4) > 33) {
              strncpy(dsc -> indexs[index].title, brk + 4, 39);
              } else {
              strncpy(dsc -> indexs[index].title, brk + 4, 33);
              strncpy((dsc -> indexs[index].title) + 
                strlen(dsc -> indexs[index].title), THE, 6);
              }
          }
      }
    }
  } while (str < (ptr + size));

  bzero(buf1, 160);
  sprintf(buf1, "Returned dsc structure for deck %d disc %d.",
    deck, disc);
  PostError(buf1);

  return dsc;
} /* Import() */

#endif /* DISPLAY_INTERFACE_CDDB */

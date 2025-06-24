// CD Player Control Class
// Suplimental GAL Protocol Object

#ifndef CD_PLAYER_CLASS
#define CD_PLAYER_CLASS

#include <jukebox.h>
#include <protocol/cdplayer.h>

CDPlayer::CDPlayer(void) {
  int i;

  for (i = 0; i < 2; i++) {
    ResetDeck(i);
    jobs[i] = NULL;
  }

  jiffies_per_second = 0;
  jiffie = 0;  

} /* CDPlayer(void) */

CDPlayer::~CDPlayer(void) { }

int CDPlayer::ResetDeck(int deck) {

  decks[deck].disc          = 0;
  decks[deck].track         = 0;
  decks[deck].playmode      = 0;
  decks[deck].state         = STATE_UNAVAILABLE;
  decks[deck].cddb          = 0;
  decks[deck].disc_length   = 0;
  decks[deck].total_tracks  = 0;
  decks[deck].track_lengths = NULL;
  decks[deck].track_time    = 0;
  decks[deck].play_time     = 0;
  decks[deck].warn_time     = 29 * jiffies_per_second;
  decks[deck].capacity      = 1;
  strcpy(decks[deck].model, "Unknown\0");
  memset(decks[deck].contents, 0, 400 * sizeof(struct content));

  return 1;
} /* ResetDeck() */

int CDPlayer::Setup(int jiffies) {
  FILE *contents;

  // Initialize the clock frequency
  jiffies_per_second = jiffies;

  // Open the S-Link device driver...
  fprintf(stdout, "Initializing S-Link device driver...  ");

  if ((slink = open("/dev/slink0", O_RDWR | O_NONBLOCK)) < 0) {
    fprintf(stderr, "Failed.\n");
    return 0; 
  }

  fprintf(stderr, "Success.\n");

  // Reload the presumed contents of the decks.
  if ((contents = fopen("contents", "rb")) == NULL) {
    fprintf(stdout, "No existing known contents, creating new contents.\n");

    if ((contents = fopen("contents", "wb")) != NULL) {
      fwrite(decks[DECK_A].contents, sizeof(struct content), 400, contents);
      fwrite(decks[DECK_B].contents, sizeof(struct content), 400, contents);
      fclose(contents);
    }
  } else {
    fseek(contents, 0, SEEK_END);

    if ((int)ftell(contents) != (2 * 400 * sizeof(content))) {
      fprintf(stdout, "Existing contents corrupt, creating new contents.\n");
      fclose(contents);

      if ((contents = fopen("contents", "wb")) != NULL) {
        fwrite(decks[DECK_A].contents, sizeof(struct content), 400, contents);
        fwrite(decks[DECK_B].contents, sizeof(struct content), 400, contents);
        fclose(contents);
      }
    } else {
      fseek(contents, 0, SEEK_SET);
      fread(decks[DECK_A].contents, sizeof(struct content), 400, contents);
      fread(decks[DECK_B].contents, sizeof(struct content), 400, contents);
      fclose(contents);
    }
  }

  // Reset their status to SLOT_UNKNOWN, because wern't no longer sure.
  for (int i = 0; i < 2; i++)
    for (int j = 0; j < 400; j++)
      decks[i].contents[j].status = SLOT_UNKNOWN;

  return 1;
} /* Setup() */

int CDPlayer::Shutdown(void) {
  int i;

  PostError("Executing CDPlayer.Shutdown()...");

  // Close the S-Link device driver...
  fprintf(stdout, "Releasing S-Link driver.\n");

  if (close(slink)) {
    fprintf(stderr, "S-Link driver could not be released.\n");
    PostError("S-Link driver could not be released.");
    return 0; 
  }

  fprintf(stdout, "Freeing allocated memory.\n\n");
  PostError("Freeing allocated memory.");  

  queue_A.Reset();
  queue_B.Reset();
  outgoing.Reset();
  messages.Reset();

  for (i = 0; i < 2; i++)
    if (jobs[i] != NULL)
      free(jobs[i]);

  while (queue_A.ListSize())
    if (queue_A.DeleteFront() != NULL) free(queue_A.DeleteFront());    

  while (queue_B.ListSize())
    if (queue_B.DeleteFront() != NULL) free(queue_B.DeleteFront());    

  while (outgoing.ListSize())
    if (outgoing.DeleteFront() != NULL) free(outgoing.DeleteFront());    

  while (messages.ListSize())
    if (messages.DeleteFront() != NULL) free(messages.DeleteFront());    

  PostError("CDPlayer.Shutdown() complete, returning 1.");

  return 1;
} /* Shutdown() */

struct content *CDPlayer::QueryContent(int deck, int disc) {
  return &(decks[deck].contents[disc - 1]);
} /* QueryContent() */

int CDPlayer::ProcessEvents(unsigned long next_jiffie) {
  struct command *job_A, *job_B;
  struct packet *out;
  int i;

  // Update our jiffie counter
  jiffie = next_jiffie;
  memset(incoming, 0, 17);

  // Maintain play timer
  for (i = 0; i < 2; i++)
    if ((decks[i].play_time >= 0) && (decks[i].state == STATE_PLAYING)) {
      decks[i].play_time++;

      if (decks[i].warn_time != 29 * jiffies_per_second)
        if (decks[i].warn_time == decks[i].track_time - decks[i].play_time)
          AddMessage(WARNTIME, i, decks[i].disc, decks[i].track);

      if (decks[i].play_time >= decks[i].track_time)
        Stop(i);
    }

  // Claim any and all packets the S-Link driver might have received.
  while ((i = read(slink, incoming, MAX_PACKET_SIZE)) > 0) {
    ProcessIncoming(i);
    memset(incoming, 0, 17);
  }

  // Pass next outgoing command to S-Link driver, these are also
  // jiffie stamped for finer accuracy.
  if (outgoing.ListSize()) {
    outgoing.Reset();

    if ((out = outgoing.Data()) != NULL)
      if (out -> jiffie <= jiffie) {
        write(slink, out -> msg, out -> size);
        free(outgoing.DeleteFront());
      }
  }

  // Use next pending packet.  If the jiffie on message is less than or
  // equal to the current jiffie.  Only one task per deck at a time,
  // the will be run as fast as possible or based on their jiffie.
  if (jobs[0] == NULL)
    if (queue_A.ListSize()) {
      queue_A.Reset();
      job_A = queue_A.Data();

      if (job_A -> jiffie <= jiffie)
        jobs[0] = queue_A.DeleteFront();
    }

  if (jobs[1] == NULL)
    if (queue_B.ListSize()) {
      queue_B.Reset();
      job_B = queue_B.Data();

      if (job_B -> jiffie <= jiffie)
        jobs[1] = queue_B.DeleteFront();
    }

  for (i = 0; i < 2; i++) {

    if (jobs[i] == NULL)
      continue;

    // Allow the job to timeout
    jobs[i] -> timeout--;

    switch (jobs[i] -> cmd) {
      case PLAY:          PlayProcess(i);         break;

      case PAUSE:         PauseProcess(i);        break;
      case STOP:          StopProcess(i);         break;

      case FASTFORWARD:
      case REWIND:
        break;

      case FADEIN:        FadeInProcess(i);       break;
      case FADEOUT:       FadeOutProcess(i);      break;
      case LOADDISC:      LoadDiscProcess(i);     break;
      case STARTUP:       StartupProcess(i);      break;

      case CDDB:
      case DISCINFO:
      case STATUS:
      case REMOTE:
        break;

      case SETDISCMEMO:   SetDiscMemoProcess(i);  break;

      case SETGROUPMEMO:
      case SETEXTENDEDMEMO:
      case STANDBY:
        break;

      case CONTENTS:      ContentsProcess(i, 0);  break;
    }
  }

  return 1;
} /* ProcessEvents() */

// Incoming information from the driver.  Either to be filtered
// and ignored, used in a current job, or passed directly back
// as a message to the control class.

int CDPlayer::ProcessIncoming(int size) {
  unsigned char type;
  int deck, reply;

  // Make a first pass setting deck information for critial packets.
  // Other functions may work with them after critical housekeeping.
  // Second byte of packet identifies type.

  deck = DeckID(incoming[0]);
  type = incoming[1];
  reply = (0x000000FF & (int)incoming[0]);

  switch (type) {

    case RSP_PLAYING:

      if (jobs[deck] != NULL)
        jobs[deck] -> timeout = DEFAULT_TIMEOUT * 10;

      break;

    case RSP_PAUSED:


      if (jobs[deck] != NULL)
        jobs[deck] -> timeout = DEFAULT_TIMEOUT * 10;

      break;

    case RSP_STOPPED:

      if (size == 2) {

        if (decks[deck].state != STATE_STOPPED)
          AddMessage(TRACKDONE, deck, decks[deck].disc, decks[deck].track);

        decks[deck].track = 0;
        decks[deck].state = STATE_STOPPED;
        decks[deck].play_time = 0;
      }

      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == STOP) {
          StopProcess(reply);
          PostError("CDPlayer.ProcessIncoming, RSP_STOPPED, cmd = STOP"
            " handled ok");
        }
 
      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == LOADDISC) {
          PostError("CDPlayer.ProcessIncoming, RSP_STOPPED, cmd = "
            "LOADDISC handled ok"); 
          LoadDiscProcess(reply);
        }

      PostError("CDPlayer.ProcessIncoming, RSP_STOPPED break");
      break;

    case RSP_PLAYING_DISC:

      if (size == 6) {

        // We're already playing a track
        if (decks[deck].state == STATE_PLAYING) {
          Stop(deck);
        } else {
          decks[deck].disc  = DiscDEC(incoming[0], incoming[2]);
          decks[deck].track = DEC(incoming[3]);
          decks[deck].track_time = ((DEC(incoming[4]) * 60) +
            DEC(incoming[5]) - 1) * jiffies_per_second;

          if (jobs[deck] != NULL)
            if (jobs[deck] -> cmd == PLAY)
              decks[deck].state = STATE_PLAYING;
            else
              if (jobs[deck] -> cmd == PAUSE)
                decks[deck].state = STATE_PAUSED;
              else
                decks[deck].state = STATE_STOPPED;
        }

        if (jobs[deck] != NULL)
          if (jobs[deck] -> cmd == PLAY)
            PlayProcess(reply);
      }

      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == PAUSE)
          PauseProcess(reply);

      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == LOADDISC)
          LoadDiscProcess(reply);
 
      break;

    case RSP_PLAYING_DISC_AT:

      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == LOADDISC)
          LoadDiscProcess(reply);

      if (decks[deck].state == STATE_PLAYING)
        decks[deck].play_time = ((DEC(incoming[4]) * 60) +
          DEC(incoming[5])) * jiffies_per_second;

      break;

    case RSP_DISPLAYING_DISC:
    case RSP_POWERON:
    case RSP_POWEROFF:
    case RSP_TRAVERSING:
      break;

    case RSP_READY:

      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == STARTUP)
          StartupProcess(reply);

      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == CONTENTS)
          ContentsProcess(reply, type);

      break;

    case RSP_UNLOADING_DISC:
      break;

    case RSP_LOADING_DISC:
      break;

    case RSP_DECK_CAPACITY:

      if (size == 4)
        decks[deck].capacity = DiscDEC(incoming[0], incoming[2]);

      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == STARTUP)
          StartupProcess(reply);

      break;

    case RSP_DECK_MODEL:

      if (size == 16)
        memcpy(decks[deck].model, incoming + 2, 14);

      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == STARTUP)
          StartupProcess(reply);

      break;

    case RSP_DECK_STATUS:

      if (size == 7) {
        decks[deck].state = ((incoming[2] & 0x10) ? 
                            STATE_STANDBY : STATE_STOPPED);
        decks[deck].playmode = incoming[3];
        decks[deck].disc  = DiscDEC(incoming[0], incoming[5]);
        decks[deck].track = DEC(incoming[6]);
      }

      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == STARTUP)
          StartupProcess(reply);

      break;

    case RSP_GET_DISC:

      if (size == 8) {
        decks[deck].total_tracks = DEC(incoming[4]);
        decks[deck].disc_length = DEC(incoming[5])*60+DEC(incoming[6]);
      }

      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == LOADDISC)
          LoadDiscProcess(reply);

      break;

    case RSP_GET_TRACK:

      if (size == 6)
        decks[deck].track_lengths[DEC(incoming[3]) - 1] =
          DEC(incoming[4]) * 60 + DEC(incoming[5]);

      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == LOADDISC)
          LoadDiscProcess(reply);

      break;

    case RSP_GET_TRACKS:
      break;

    case RSP_MEMO_SET:
    case RSP_MEMO_SET_1:

      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == SETDISCMEMO)
          SetDiscMemoProcess(reply);

      break;

    case RSP_GET_MEMO:
    case RSP_GET_GROUP:
      break;

    case RSP_29_REMAIN:

      if (decks[deck].warn_time == 29 * jiffies_per_second)
        AddMessage(WARNTIME, deck, decks[deck].disc, decks[deck].track);

      break;

    case RSP_DOOR_OPEN:
      // Should flag all of contents as presumed correct
      // Why?  JRB
      break;

    case RSP_DOOR_CLOSED:
      // Should automatically rescan everything.
      break;

    case RSP_NO_DISC:

      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == STARTUP)
          StartupProcess(reply);

      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == LOADDISC)
          LoadDiscProcess(reply);

      break;

    case RSP_DISC_UNLOADED:

      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == LOADDISC)
          LoadDiscProcess(reply);

      break;

    case RSP_TRACK_UNLOADED:
    case RSP_DUPLICATE_CMD:

      if (jobs[deck] != NULL) 
        if (jobs[deck] -> cmd == LOADDISC)
          LoadDiscProcess(reply);

      if (jobs[deck] != NULL) 
        if (jobs[deck] -> cmd == FADEIN)
          FadeInProcess(reply);

      if (jobs[deck] != NULL) 
        if (jobs[deck] -> cmd == FADEOUT)
          FadeOutProcess(reply);

      break;

    case RSP_LOADED_DISCS_1:
    case RSP_LOADED_DISCS_2:
    case RSP_LOADED_DISCS_3:
    case RSP_LOADED_DISCS_4:

      if (jobs[deck] != NULL)
        if (jobs[deck] -> cmd == CONTENTS)
          ContentsProcess(reply, type);

      break;

    case RSP_INVALID_CMD:
      break;
  }

  return 1;
} /* ProcessIncoming() */

// Pointer to all active deck information
struct deck *CDPlayer::QueryDeck(int deck) { return &decks[deck]; }

// Pointer to all active deck information
struct command *CDPlayer::QueryJob(int deck) { return jobs[deck]; }

// Remove the active job from a deck
int CDPlayer::StopJob(int deck) {

  if (jobs[deck] != NULL) {
    free(jobs[deck]);
    jobs[deck] = NULL;
    return 1;
  }

  return 0;
} /* StopJob() */

// Flush anything pending by deck.
int CDPlayer::FlushMessages(int deck) {
  struct message *msg;
  int i = 0, flushed = 0;

  PostError("Entered CDPlayer.FlushMessages");

  if (deck == -1) {
    flushed = messages.ListSize();

    while (messages.ListSize())
      if ((msg = messages.DeleteFront()) != NULL)
        free(msg);

    return flushed;
  }

  messages.Reset();

  while (i < messages.ListSize()) {
    msg = messages.Data();

    if (msg -> deck == deck) {
      messages.DeleteAt();
      if (msg != NULL) free(msg);
      flushed++;
    } else {
      messages.Next();
      i++;
    }
  }

  return flushed;
} /* FlushMessages() */

// Users are responsible for free()'ing the message
struct message *CDPlayer::QueryMessage() { 

  if (messages.ListSize())
    return messages.DeleteFront();

  return NULL;
} /* QueryMessage()*/

// Users are responsible for free()'ing the message
struct message *CDPlayer::PeekMessage() { 

  if (messages.ListSize())
    return messages.Index(0);

  return NULL;
} /* QueryMessage()*/

char *CDPlayer::QueryRunTime(int deck) {
  static char run[85], color[16];
  int min, sec, frame;

  if (decks[deck].track_time > 0) {

    if ((frame = decks[deck].play_time) < 0)
      frame = 0;

    if ((decks[deck].track_time - frame) < (30 * jiffies_per_second))
      sprintf(color, RESET_ASC BG_BLUE_ASC BOLD_ASC RED_ASC);
    else
      if ((decks[deck].track_time - frame) < (60 * jiffies_per_second))
        sprintf(color, BG_BLUE_ASC BOLD_ASC YELLOW_ASC);
      else
        sprintf(color, RESET_ASC BG_BLUE_ASC WHITE_ASC);
  } else {
    frame = 0;
    sprintf(color, RESET_ASC BG_BLUE_ASC WHITE_ASC);
  }

  min   = frame / (jiffies_per_second * 60);
  frame = frame % (jiffies_per_second * 60);
  sec   = frame / jiffies_per_second;
  frame = (frame % jiffies_per_second) * 10 / jiffies_per_second;

  sprintf(run, BG "%s%.02d" FG ":%s%.02d" FG ".%s%d" RESET_ASC BG,
          BLUE, color, min, WHITE, color, sec, WHITE, color, frame,
          BLUE);
  return run;
} /* QueryRunTime() */

char *CDPlayer::QueryRemainTime(int deck) {
  static char remain[85], color[16];
  int min, sec, frame;

  if (decks[deck].track_time > 0) {

    if ((frame = decks[deck].track_time - decks[deck].play_time) < 0)
      frame = 0;

    if (frame < (30 * jiffies_per_second))
      sprintf(color, RESET_ASC BG_BLUE_ASC BOLD_ASC RED_ASC);
    else
      if (frame < (60 * jiffies_per_second))
        sprintf(color, BOLD_ASC BG_BLUE_ASC YELLOW_ASC);
      else
        sprintf(color, RESET_ASC BG_BLUE_ASC WHITE_ASC);
  } else {
    frame = 0;
    sprintf(color, RESET_ASC BG_BLUE_ASC WHITE_ASC);
  }

  min   = frame / (jiffies_per_second * 60);
  frame = frame % (jiffies_per_second * 60);
  sec   = frame / jiffies_per_second;
  frame = (frame % jiffies_per_second) * 10 / jiffies_per_second;

  sprintf(remain, BG "%s%.02d" FG ":%s%.02d" FG ".%s%d" RESET_ASC BG,
          BLUE, color, min, WHITE, color, sec, WHITE, color, frame, BLUE);
  return remain;
} /* QueryRemainTime() */

// Messages will be malloc()'ed automagically
int CDPlayer::AddMessage(int id, int deck, int disc, int track,
                         int min, int sec, void *ptr) {
  struct message *msg;

  msg = (struct message *)calloc(sizeof(struct message), 1);
  msg -> id    = id;
  msg -> deck  = deck;
  msg -> disc  = disc;
  msg -> track = track;
  msg -> min   = min;
  msg -> sec   = sec;
  msg -> ptr   = ptr;
  messages.InsertRear(msg);

  return 1;
} /* AddMessage() */

int CDPlayer::AddCommand(int deck, struct command *command) {
  int i, size;
  struct command *ptr;

  if (command == NULL)
    return 0;

  if (deck == DECK_A) {
    size = queue_A.ListSize();
    queue_A.Reset();

    for (i = 0; i < size; i++) {
      ptr = queue_A.Data();

      if (command -> jiffie < ptr -> jiffie) {
        queue_A.InsertAt(command);
        return 1;
      }

      queue_A.Next();
    }

    queue_A.InsertRear(command);
    return 1;
  }

  if (deck == DECK_B) {
    size = queue_B.ListSize();
    queue_B.Reset();

    for (i = 0; i < size; i++) {
      ptr = queue_B.Data();

      if (command -> jiffie < ptr -> jiffie) {
        queue_B.InsertAt(command);
        return 1;
      }

      queue_B.Next();
    }

    queue_B.InsertRear(command);
    return 1;
  }

  return 0;
} /* AddCommand() */


int CDPlayer::AddOutgoing(struct packet *out) {
  int i, size;
  struct packet *ptr;

  if (out == NULL)
    return 0;

  size = outgoing.ListSize();
  outgoing.Reset();

  for (i = 0; i < size; i++) {
    ptr = outgoing.Data();

    if (out -> jiffie < ptr -> jiffie) {
      outgoing.InsertAt(out);
      return 1;
    }

    outgoing.Next();
  }

  outgoing.InsertRear(out);
  return 1;
} /* AddOutgoing() */

// Converts Sony CD Identifier to useable deck reference, 0x98 -> 0
unsigned char CDPlayer::DeckID(unsigned char deck) {

  switch (deck) {
    case RSP_DECK_A1:  case RSP_DECK_A2:  return 0;
    case RSP_DECK_B1:  case RSP_DECK_B2:  return 1;
  }

  return 0xFF;
} /* DeckID() */

// Converts a integer (0-99) to packed BCD format
unsigned char CDPlayer::BCD(int val) {
  unsigned char upper, lower;

  val   = val % 100;
  upper = (unsigned char)((val / 10) << 4);
  lower = (unsigned char)(val % 10);

  return (upper | lower);
} /* BCD() */

// Converts a BCD char (0xXX) to an integer
int CDPlayer::DEC(unsigned char bcd) {
  unsigned char upper, lower;

  upper = ((bcd & 0xF0) >> 4) * 10;
  lower = bcd & 0x0F;

  return (int)(upper + lower);
} /* DEC() */

unsigned char CDPlayer::DiscBCD(int val) {

  if (val < 100)
    return BCD(val);

  if (val <= 200)
    return (unsigned char)((val - 100) + 0x9A);

  return 0;
} /* DiscBCD() */

int CDPlayer::DiscDEC(unsigned char device, unsigned char bcd) {

  if (device < 0x9A) {
    if (bcd < 0x9A)
      return DEC(bcd);

    if (bcd <= 0xFE)
      return (int)((bcd - 0x9A) + 100);
  } else {
    if (bcd <= 200)
      return (200 + (int)bcd);
  }
  
  return 0;
} /* DiscDEC() */

// Pass a -1 to exist/status to preserve existing value.
int CDPlayer::SetContent(int deck, int disc, int exist,
                         int status, long cddb) {

  if (exist >= 0)
    decks[deck].contents[disc].slot = exist;

  if (status >= 0)
    decks[deck].contents[disc].status = status;

  decks[deck].contents[disc].cddb = cddb;
  return 1;
} /* SetContent() */

int CDPlayer::SetWarnTime(int deck, int warn) { 
  return (decks[deck].warn_time = warn);
} /* SetWarnTime() */

int CDPlayer::Startup(void) {
  struct command *command;

  command = (struct command *)calloc(sizeof(struct command), 1);
  command -> jiffie  = jiffie + 1;
  command -> timeout = DEFAULT_TIMEOUT;
  command -> cmd     = STARTUP;
  command -> state   = 1;

  return AddCommand(DECK_A, command);
} /* Startup() */

int CDPlayer::Play(int deck, int d, int t, int m, int s, int f) {
  struct command *command;

  command = (struct command *)calloc(sizeof(struct command), 1);
  command -> jiffie  = jiffie + 1;
  command -> timeout = DEFAULT_TIMEOUT;
  command -> cmd     = PLAY;
  command -> disc    = d;
  command -> track   = t;
  command -> min     = m;
  command -> sec     = s;
  command -> frame   = f;
  command -> state   = 1;

  return AddCommand(deck, command);
} /* Play() */

int CDPlayer::Pause(int deck, int d, int t, int m , int s, int f) {
  struct command *command;

  command = (struct command *)calloc(sizeof(struct command), 1);
  command -> jiffie  = jiffie + 1;
  command -> timeout = DEFAULT_TIMEOUT;
  command -> cmd     = PAUSE;
  command -> disc    = d;
  command -> track   = t;
  command -> min     = m;
  command -> sec     = s;
  command -> frame   = f;
  command -> state   = 1;

  return AddCommand(deck, command);
} /* Pause() */

int CDPlayer::Stop(int deck, int delay) { 
  struct command *command;

  command = (struct command *)calloc(sizeof(struct command), 1);
  if (delay == 0) {
    command -> jiffie  = jiffie + 1;
    } else {
    command -> jiffie  = jiffie + 1 + (delay * jiffies_per_second);
    // The plus 1 above is for paranoia's sake...JRB
    }
  command -> timeout = DEFAULT_TIMEOUT;
  command -> cmd     = STOP;
  command -> state   = 1;

  return AddCommand(deck, command);
} /* stop() */

int CDPlayer::FastForward(int, int, int) { return 1; }
int CDPlayer::Rewind(int, int, int) { return 1; }

int CDPlayer::FadeIn(int deck, int delay) {
  struct command *command;
  
  command = (struct command *)calloc(sizeof(struct command), 1);
  command -> jiffie  = jiffie + 1;
  command -> timeout = jiffies_per_second * (delay + 1);
  command -> cmd     = FADEIN;
  command -> sec     = delay;
  command -> state   = 1;   
      
  return AddCommand(deck, command);
} /* FadeIn */
      
int CDPlayer::FadeOut(int deck, int delay) {
  struct command *command;
      
  command = (struct command *)calloc(sizeof(struct command), 1);
  command -> jiffie  = jiffie + 1;
  command -> timeout = jiffies_per_second * (delay + 1);
  command -> cmd     = FADEOUT;
  command -> sec     = delay;
  command -> state   = 1;   
      
  return AddCommand(deck, command);
} /* FadeOut */

int CDPlayer::LoadDisc(int deck, int d) {
  struct command *command;

  command = (struct command *)calloc(sizeof(struct command), 1);
  command -> jiffie  = jiffie + 1;
  command -> timeout = DEFAULT_TIMEOUT * 60;
  command -> cmd     = LOADDISC;
  command -> disc    = d;
  command -> state   = 1;

  return AddCommand(deck, command);
} /* LoadDisc() */

int CDPlayer::RemoteMode(int, int) { return 1; }
int CDPlayer::Standby(int, int) { return 1; }

int CDPlayer::Contents(int deck) {
  struct command *command;

  command = (struct command *)calloc(sizeof(struct command), 1);
  command -> jiffie  = jiffie + 1;
  command -> timeout = DEFAULT_TIMEOUT;
  command -> cmd     = CONTENTS;
  command -> state   = 1;

  return AddCommand(deck, command);
} /* Contents() */

int CDPlayer::SetDiscMemo(int deck, int disc, char *disc_title) {   
  struct command *command;

  command = (struct command *)calloc(sizeof(struct command), 1);
  command -> ptr = (char *)calloc(14, 1);
  memset((char *)command -> ptr, 0x20, 13);

  if (disc_title == NULL)
    strncpy((char *)command -> ptr, "CDDB Unavail", 12);
  else
    strncpy((char *)command -> ptr, disc_title, 
            (strlen(disc_title) < 14) ? strlen(disc_title) : 13);

  command -> jiffie  = jiffie + 1;
  command -> timeout = DEFAULT_TIMEOUT;
  command -> cmd     = SETDISCMEMO;
  command -> disc    = disc;
  command -> state   = 1;

  return AddCommand(deck, command);
} /* SetDiscMemo() */

int CDPlayer::GetDiscMemo(int, int, void *) { return 1; }
int CDPlayer::SetGroupMemo(int, int, void *) { return 1; }
int CDPlayer::GetGroupMemo(int, int, void *) { return 1; }
int CDPlayer::SetExtendedDiscMemo(int, int, int, void *) { return 1; }
int CDPlayer::GetExtendedDiscMemo(int, int, int, void *) { return 1; }

// Deck is 0 or 1 to reference an specific deck, if deck is >= 2 a
// pending response from the deck is available for the function in
// message.  The deck value with represent the source deck.

int CDPlayer::PlayProcess(int deck) { 
  unsigned char id = CMD_DECK_A1, bcd;
  struct packet *pkt;
  int job = deck;

  // Valid play states:
  //   1 - Issuing play command to driver
  //   2 - Waiting for simple play response
  //   3 - Waiting for complex play response

  if (deck >= 2)
    job = DeckID(deck);

  if ((jobs[job] -> state > 3) || (jobs[job] -> state < 1)) {

    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
    }

    return 1;
  }

  // Drat, go back to state 1 and try again.
  if (jobs[job] -> timeout <= 0) {
    jobs[job] -> state = 1;
    jobs[job] -> timeout = DEFAULT_TIMEOUT;
  }

  if (job == 1)
    id = CMD_DECK_B1;

  if (jobs[job] -> state == 1) {
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;

    if (!jobs[job] -> disc) {
      // No disc, simple play command
      pkt -> msg[1] = CMD_PLAY;
      pkt -> size = 2;
      jobs[job] -> state = 2;
    } else {

      // Specified disk, optional track.  For now we'll ignore any 
      // fancy stuff to seek to a point in the track.  We should be
      // able to do this down to 5 frame accuracy or so.  Closer if
      // we can crank the jiffies on a faster box.

      if ((bcd = DiscBCD(jobs[job] -> disc))) {
        pkt -> msg[2] = bcd;
      } else {
        pkt -> msg[2] = (unsigned char)(jobs[job] -> disc - 200);
        id += 0x03;
      }

      pkt -> msg[1] = CMD_PLAY_DISC;
      pkt -> msg[3] = BCD(jobs[job] -> track);
      pkt -> size = 4;

      jobs[job] -> state = 3;
      decks[job].state = STATE_CUEING;
    }

    pkt -> msg[0] = id;
    AddOutgoing(pkt);

    return 1;
  }

  // Response from deck, deck = deck ID
  if ((jobs[job] -> state >= 2) && (deck >= 2)) {
    
    // Post a message for our user class
    AddMessage(PLAYING, job, jobs[job] -> disc, jobs[job] -> track);
    decks[job].play_time = 0;

    // Initiate verbose mode to keep track counter in sync
//    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
//    pkt -> jiffie = jiffie + 20;
//    pkt -> size = 2;

//    pkt -> msg[0] = id;
//    pkt -> msg[1] = CMD_VERBOSE;

//    AddOutgoing(pkt);

    // Remove the play command now that its complete
    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
    }

    return 1;
  }

  return 0;
} /* PlayProcess() */

int CDPlayer::PauseProcess(int deck) {
  unsigned char id = CMD_DECK_A1, bcd;
  struct packet *pkt;
  int job = deck;

  // Valid pause states:
  //   1 - Issuing pause command to driver
  //   2 - Waiting for simple pause response
  //   3 - Waiting for complex play response

  if (deck >= 2)
    job = DeckID(deck);

  if ((jobs[job] -> state > 3) || (jobs[job] -> state < 1)) {
    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
    }

    return 1;
  }

  // Drat, go back to state 1 and try again.
  if (jobs[job] -> timeout <= 0) {
    jobs[job] -> state = 1;
    jobs[job] -> timeout = DEFAULT_TIMEOUT;
  }

  if (job == 1)
    id = CMD_DECK_B1;

  if (jobs[job] -> state == 1) {
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;

    if (!jobs[job] -> disc) {
      // No disc, simple pause command
      pkt -> msg[1] = CMD_PAUSE;
      pkt -> size = 2;
      jobs[job] -> state = 2;
    } else {

      // Specified disk, optional track.  For now we'll ignore any 
      // fancy stuff to seek to a point in the track.  We should be
      // able to do this down to 5 frame accuracy or so.  Closer if
      // we can crank the jiffies on a faster box.

      if ((bcd = DiscBCD(jobs[job] -> disc))) {
        pkt -> msg[2] = bcd;
      } else {
        pkt -> msg[2] = (unsigned char)(jobs[job] -> disc - 200);
        id += 0x03;
      }

      pkt -> msg[1] = CMD_PAUSE_DISC;
      pkt -> msg[3] = BCD(jobs[job] -> track);
      pkt -> size = 4;
      jobs[job] -> state = 3;
      decks[job].state = STATE_CUEING;
    }

    pkt -> msg[0] = id;
    AddOutgoing(pkt);
    return 1;
  }

  // Response from deck, deck = deck ID
  if ((jobs[job] -> state >= 2) && (deck >= 2)) {
    
    // Post a message for our user class
    AddMessage(PAUSED, job, decks[job].disc, decks[job].track);

    // Remove the pause command now that its complete
    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
    }

    return 1;
  }

  return 0;
} /* PauseProcess() */

int CDPlayer::StopProcess(int deck) {
  unsigned char id = CMD_DECK_A1;
  struct packet *pkt;
  int job = deck;

  // Valid stop states:
  //   1 - Issuing stop command to driver
  //   2 - Waiting stop response

  if (deck >= 2)
    job = DeckID(deck);

  if ((jobs[job] -> state > 2) || (jobs[job] -> state < 1) ||
      (jobs[job] -> timeout <= 0)) {
    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
    }

    return 1;
  }

  if (job == 1)
    id = CMD_DECK_B1;

  if (jobs[job] -> state == 1) {
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;
    pkt -> msg[0] = id;
    pkt -> msg[1] = CMD_STOP;
    pkt -> size = 2;
    jobs[job] -> state = 2;

    AddOutgoing(pkt);
    return 1;
  }

  if ((jobs[job] -> state == 2) && (deck >= 2)) {
    
    // Post a message for our user class
    AddMessage(STOPPED, job);
    decks[job].play_time = 0;

    // Put us back in brief mode to cut down on the spam.
//    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
//    pkt -> jiffie = jiffie + 1;
//    pkt -> msg[0] = id;
//    pkt -> msg[1] = CMD_BRIEF;
//    pkt -> size = 2;

//    AddOutgoing(pkt);

    // Remove the stop command now that its complete
    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
    }

    return 1;
  }

  return 0;
} /* StopProcess() */

int CDPlayer::FastForwardProcess(int deck) { return 1; }
int CDPlayer::RewindProcess(int deck) { return 1; }

int CDPlayer::FadeInProcess(int deck) {
  unsigned char id = CMD_DECK_A1;
  struct packet *pkt;
  int job = deck;
    
  // Valid stop states:
  //   1 - Issuing fade in command to driver
  //   2 - Waiting fade in duplicate error response
  //   3 - Check for sucessful fade, issue state message
   
  if (deck >= 2)
    job = DeckID(deck);
      
  if ((jobs[job] -> state > 3) || (jobs[job] -> state < 1)) {
    PostError("CDPlayer.FadeInProcess, state confusion! free job.");
    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
      PostError("CDPlayer.FadeInProcess, job freed.");
    }
  
    return 1;
  } 

  if (job == 1)
    id = CMD_DECK_B1;
  
  if (jobs[job] -> state == 1) {
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;
    pkt -> msg[0] = id;
    pkt -> msg[1] = CMD_FADEIN; 
    pkt -> msg[2] = jobs[job] -> sec;
    pkt -> size   = 3;
    jobs[job] -> state = 2;
    jobs[job] -> timeout = jiffies_per_second * (jobs[job] -> sec + 1);
    
    AddOutgoing(pkt);
    AddMessage(FADINGIN, job);
    return 1;
  }

  if ( (jobs[job] -> state == 2) && (jobs[job] -> timeout <= 0)) {
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;
    pkt -> msg[0] = id;
    pkt -> msg[1] = CMD_FADEIN; 
    pkt -> msg[2] = jobs[job] -> sec;
    pkt -> size   = 3;
    jobs[job] -> state = 3;
    jobs[job] -> timeout = DEFAULT_TIMEOUT;    

    AddOutgoing(pkt);
    return 1;
  }    
        
  if ((jobs[job] -> state == 3) && (jobs[job] -> timeout <= 0)) {
    PostError("FadeIn failed");

    AddMessage(FADEINFAIL, job);
    
    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
      PostError("FadeIn job freed sucessfully");
    }
     
    return 1;
  }

  if ((jobs[job] -> state == 3) && (deck >= 2)) {
    PostError("FadeIn complete");  

    AddMessage(FADEDIN, job);
    
    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
      PostError("FadeIn job freed sucessfully");
    }
     
    return 1;
  }
    
  return 0;
} /* FadeInProcess() */
    
int CDPlayer::FadeOutProcess(int deck) {
  unsigned char id = CMD_DECK_A1;
  struct packet *pkt;
  int job = deck;
    
  // Valid stop states:
  //   1 - Issuing fade out command to driver
  //   2 - Waiting fade out duplicate error response
  //   3 - Check for sucessful fade, issue state message
   
  if (deck >= 2)
    job = DeckID(deck);
      
  if ((jobs[job] -> state > 3) || (jobs[job] -> state < 1)) {
    PostError("CDPlayer.FadeOutProcess, state confusion! free job.");
    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
      PostError("CDPlayer.FadeOutProcess, job freed.");
    }
  
    return 1;
  } 

  if (job == 1)
    id = CMD_DECK_B1;
  
  if (jobs[job] -> state == 1) {
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;
    pkt -> msg[0] = id;
    pkt -> msg[1] = CMD_FADEOUT; 
    pkt -> msg[2] = jobs[job] -> sec;
    pkt -> size   = 3;
    jobs[job] -> state = 2;
    jobs[job] -> timeout = jiffies_per_second * (jobs[job] -> sec + 1);
    
    AddOutgoing(pkt);
    AddMessage(FADINGOUT, job);
    return 1;
  }

  if ( (jobs[job] -> state == 2) && (jobs[job] -> timeout <= 0)) {
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;
    pkt -> msg[0] = id;
    pkt -> msg[1] = CMD_FADEOUT; 
    pkt -> msg[2] = jobs[job] -> sec;
    pkt -> size   = 3;
    jobs[job] -> state = 3;
    jobs[job] -> timeout = DEFAULT_TIMEOUT;    

    AddOutgoing(pkt);
    return 1;
  }    
        
  if ((jobs[job] -> state == 3) && (jobs[job] -> timeout <= 0)) {
    PostError("FadeOut failed");

    AddMessage(FADEOUTFAIL, job);
    
    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
      PostError("FadeOut job freed sucessfully");
    }
     
    return 1;
  }

  if ((jobs[job] -> state == 3) && (deck >= 2)) {
    PostError("FadeOut complete");  

    AddMessage(FADEDOUT, job);
    
    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
      PostError("FadeOut job freed sucessfully");
    }
     
    return 1;
  }
    
  return 0;
} /* FadeOutProcess() */

int CDPlayer::StartupProcess(int deck) {
  unsigned char id = CMD_DECK_A1;
  struct packet *pkt;
  struct command *command;
  int job = deck;

  // Valid startup states:
  //   1 - Issue deck model command
  //   2 - Issue deck capacity command
  //   3 - Issue deck status command
  //   4 - Deck off -> 5; Deck on -> 6
  //   5 - Wait for power on
  //   6 - Remove job; Post response

  if (deck >= 2)
    job = DeckID(deck);

  // Invalid state for job or job has expired; remove current job
  if ((jobs[job] -> state > 6) || (jobs[job] -> state < 1) ||
      (jobs[job] -> timeout <= 0)) {

    // Post a negitive deck ID to indicate the deck is not responding
    AddMessage(STARTEDUP, -1 * job);
    decks[job].state = STATE_UNAVAILABLE;

    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
    }

    return 1;
  }

  if (job == 1)
    id = CMD_DECK_B1;

  if ((jobs[job] -> state == 1) && (deck < 2)) {
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;
    pkt -> msg[0] = id;
    pkt -> msg[1] = CMD_STOP;
    pkt -> size = 2;
    AddOutgoing(pkt);

    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 20;
    pkt -> msg[0] = id;
    pkt -> msg[1] = CMD_GET_DECK_MODEL;
    pkt -> size = 2;

    jobs[job] -> state = 2;
    jobs[job] -> timeout = DEFAULT_TIMEOUT;

    AddOutgoing(pkt);
    return 1;
  }

  if ((jobs[job] -> state == 2) && (deck >= 2)) {
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;
    pkt -> msg[0] = id;
    pkt -> msg[1] = CMD_GET_DECK_CAPACITY;
    pkt -> size = 2;

    jobs[job] -> state = 3;
    jobs[job] -> timeout = DEFAULT_TIMEOUT;

    AddOutgoing(pkt);
    return 1;
  }

  if ((jobs[job] -> state == 3) && (deck >= 2)) {
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;
    pkt -> msg[0] = id;
    pkt -> msg[1] = CMD_GET_DECK_STATUS;
    pkt -> size = 2;

    jobs[job] -> state = 4;
    jobs[job] -> timeout = DEFAULT_TIMEOUT;

    AddOutgoing(pkt);
    return 1;
  }

  if ((jobs[job] -> state == 4) && (deck >= 2)) {
    if (decks[job].state == STATE_STANDBY) {
      pkt = (struct packet *)calloc(sizeof(struct packet), 1);
      pkt -> jiffie = jiffie + 1;
      pkt -> msg[0] = id;
      pkt -> msg[1] = CMD_POWERON;
      pkt -> size = 2;
 
      jobs[job] -> state = 5;
      jobs[job] -> timeout = DEFAULT_TIMEOUT * 5;

      AddOutgoing(pkt);
    } else {
      jobs[job] -> state = 6;
    }

    return 1;
  }

  if ((jobs[job] -> state == 5) && (deck >= 2)) {
    if (incoming[1] == RSP_READY)
      jobs[job] -> state = 6;

    return 1;
  }

  if ((jobs[job] -> state == 6) && (deck < 2)) {
    AddMessage(STARTEDUP, job);

    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
    }

    if (job == DECK_A) {
      command = (struct command *)calloc(sizeof(struct command), 1);
      command -> jiffie  = jiffie + 1;
      command -> timeout = DEFAULT_TIMEOUT;
      command -> cmd     = STARTUP;
      command -> state   = 1;
      AddCommand(DECK_B, command);
    }

    return 1;
  }

  return 0;
} /* StartupProcess() */

int CDPlayer::ShutdownProcess(int deck) { return 1; }

int CDPlayer::LoadDiscProcess(int deck) {
  unsigned char id = CMD_DECK_A1, bcd;
  struct packet *pkt;
  char title[14];
  int i, j, length, job = deck;
  static int track[2] = { -1, -1 };
  FILE *contents;

  // Valid Load Disc States:
  //  1 - Wait till clear; Issue Stop, then Play/Pause
  //  2 - Wait till response and clear; Issue Query Disc
  //  3 - Wait till response and clear; Issue 1st Track Request; Lock Bus
  //  4 - Issue 2 -> N Track Requests
  //  5 - Calculate CDDB; Issue Stop; Set Bus to be Clear; Remove Job
  //  6 - Wait till deck is stopped, then post user message.

  // S-Link Bus Activity (stored in 'tracks')
  // -3 - Prepping disc
  // -2 - Prepped disc (waiting for clear)
  // -1 - Bus not-locked (is clear)
  //  N - Running current track N

  if (deck >= 2)
    job = DeckID(deck);

  if (job == 1)
    id = CMD_DECK_B1;

/*
  fprintf(stdout, SAVE_LOC LOC "%d %2d %3d : %d %2d %3d" CLEAR_EOL LOAD_LOC, 8, 2,
          (jobs[0] != NULL) ? jobs[0] -> state : 0, track[0],
          (jobs[0] != NULL) ? jobs[0] -> timeout : 0,
          (jobs[1] != NULL) ? jobs[1] -> state : 0, track[1],
          (jobs[1] != NULL) ? jobs[1] -> timeout : 0);
*/

  // Purge the job, Unknown state or timeout.
  if ((jobs[job] -> state > 6) || (jobs[job] -> state < 1) ||
      (jobs[job] -> timeout <= 0)) {

    // What's the deal with loading the disc?  Send it again.
    if (jobs[job] -> state == 2) {
      pkt = (struct packet *)calloc(sizeof(struct packet), 1);
      pkt -> jiffie = jiffie + 1;
      pkt -> msg[1] = CMD_PAUSE_DISC;
      pkt -> msg[3] = BCD(1);
      pkt -> size = 4;

      if ((bcd = DiscBCD(jobs[job] -> disc))) {
        pkt -> msg[2] = bcd;
      } else {
        pkt -> msg[2] = (unsigned char)(jobs[job] -> disc - 200);
        id += 0x03;
      }

      pkt -> msg[0] = id;
      AddOutgoing(pkt);

      jobs[job] -> timeout = DEFAULT_TIMEOUT * 8;
      return 1;
    }

    // We didn't get the Disc Query response, send it again.
    if (jobs[job] -> state == 3) {
      pkt = (struct packet *)calloc(sizeof(struct packet), 1);
      pkt -> jiffie = jiffie + 1;
      pkt -> msg[1] = CMD_GET_DISC;

      if ((bcd = DiscBCD(jobs[job] -> disc))) {
        pkt -> msg[2] = bcd;
      } else {
        pkt -> msg[2] = (unsigned char)(jobs[job] -> disc - 200);
        id += 0x03;
      }

      pkt -> msg[0] = id;
      pkt -> size = 3;
      AddOutgoing(pkt);

      jobs[job] -> timeout = DEFAULT_TIMEOUT * 5;
      return 1;
    }

    // We never got our Track Query response, send it again.
    if (jobs[job] -> state == 4) {
      pkt = (struct packet *)calloc(sizeof(struct packet), 1);
      pkt -> jiffie = jiffie + 1;
      pkt -> msg[1] = CMD_GET_TRACK;
      pkt -> msg[3] = BCD(track[job]);
      pkt -> size = 4;

      if ((bcd = DiscBCD(jobs[job] -> disc))) {
        pkt -> msg[2] = bcd;
      } else {
        pkt -> msg[2] = (unsigned char)(jobs[job] -> disc - 200);
        id += 0x03;
      }

      pkt -> msg[0] = id;
      AddOutgoing(pkt);

      jobs[job] -> timeout = DEFAULT_TIMEOUT;
      return 1;
    }

    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
    }

    track[job] = -1;
    return 1;
  }

  if (jobs[job] -> state == 1) {
    track[job] = -1;

    // Spin our wheels until the other deck releases the bus.
    if (track[!job] >= 0)
      return 1;

    // The bus is clear;  Reset the deck status appropriately
    decks[job].disc  = 0;
    decks[job].track = 0;
    decks[job].state = STATE_CUEING;
    decks[job].play_time = 0;

    // dump our packets while we can.
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;
    pkt -> msg[0] = id;
    pkt -> msg[1] = CMD_STOP;
    pkt -> size = 2;
    AddOutgoing(pkt);

    // Issue a disc play to load the disc
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 10;
    pkt -> msg[1] = CMD_PAUSE_DISC;
    pkt -> msg[3] = BCD(1);
    pkt -> size = 4;

    if ((bcd = DiscBCD(jobs[job] -> disc))) {
      pkt -> msg[2] = bcd;
    } else {
      pkt -> msg[2] = (unsigned char)(jobs[job] -> disc - 200);
      id += 0x03;
    }

    pkt -> msg[0] = id;
    AddOutgoing(pkt);

    // Post a message for our user class
    AddMessage(CUEING, job, jobs[job] -> disc, 1);

    track[job] = -3;
    jobs[job] -> state = 2;
    jobs[job] -> timeout = DEFAULT_TIMEOUT * 8;
    return 1;
  }

  if (jobs[job] -> state == 2) {
    // No Disc, Purge Job.
    if ((incoming[1] == RSP_NO_DISC) ||
        (incoming[1] == RSP_DUPLICATE_CMD)) {

      // Clear the Memo on the Deck, no disk present.  JRB
      bzero(title, 14);
      SetDiscMemo(job, jobs[job] -> disc, title);

      // Post a message for our user class
      AddMessage(NODISC, job, jobs[job] -> disc);

      // Update known deck contents
      decks[job].contents[jobs[job] -> disc - 1].cddb = 0;

      // Going from CASE to EMPTY.
      switch (decks[job].contents[jobs[job] -> disc - 1].slot) {
        default:
          decks[job].contents[jobs[job] -> disc - 1].slot   = SLOT_UNKNOWN;
          decks[job].contents[jobs[job] -> disc - 1].status = SLOT_UNKNOWN;
          break;
        case SLOT_EMPTY:
          decks[job].contents[jobs[job] -> disc - 1].slot   = SLOT_EMPTY;
          decks[job].contents[jobs[job] -> disc - 1].status = SLOT_MATCHED;
          break;
        case SLOT_FULL:
          decks[job].contents[jobs[job] -> disc - 1].slot   = SLOT_EMPTY;
          decks[job].contents[jobs[job] -> disc - 1].status = SLOT_CORRECTED;
          break;
        case SLOT_UNKNOWN:
          decks[job].contents[jobs[job] -> disc - 1].slot   = SLOT_EMPTY;
          decks[job].contents[jobs[job] -> disc - 1].status = SLOT_CORRECTED;
      }          

      if ((contents = fopen("contents", "wb")) != NULL) {
        fwrite(decks[DECK_A].contents, sizeof(struct content), 400, contents);
        fwrite(decks[DECK_B].contents, sizeof(struct content), 400, contents);
        fclose(contents);
      }

      if (jobs[job] != NULL) {
        free(jobs[job]);
        jobs[job] = NULL;
      }

      track[job] = -1;
      return 1;
    }

    // We've got a disc atleast; Onward
    if ((incoming[1] == RSP_PLAYING_DISC) ||
        (incoming[1] == RSP_PLAYING_DISC_AT))
      track[job] = -2;

    // The bus is busy, or we're not prepped
    if ((track[!job] >= 0) || (track[job] != -2))
      return 1;

    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;
    pkt -> msg[0] = id;
    pkt -> msg[1] = CMD_STOP;
    pkt -> size = 2;
    AddOutgoing(pkt);

    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 10;
    pkt -> msg[1] = CMD_GET_DISC;

    if ((bcd = DiscBCD(jobs[job] -> disc))) {
      pkt -> msg[2] = bcd;
    } else {
      pkt -> msg[2] = (unsigned char)(jobs[job] -> disc - 200);
      id += 0x03;
    }

    pkt -> msg[0] = id;
    pkt -> size = 3;
    AddOutgoing(pkt);

    track[job] = -3;
    jobs[job] -> state = 3;
    jobs[job] -> timeout = DEFAULT_TIMEOUT * 5;
    return 1;
  }

  if (jobs[job] -> state == 3) {

    if (incoming[1] == RSP_GET_DISC)
      track[job] = -2;

    // The bus is busy, or we're not prepped
    if ((track[!job] >= 0) || (track[job] != -2))
      return 1;

/*
    fprintf(stdout, SAVE_LOC LOC "%d %2d %3d : %d %2d %3d" CLEAR_EOL LOAD_LOC, 9, 2,
          (jobs[0] != NULL) ? jobs[0] -> state : 0, track[0],
          (jobs[0] != NULL) ? jobs[0] -> timeout : 0,
          (jobs[1] != NULL) ? jobs[1] -> state : 0, track[1],
          (jobs[1] != NULL) ? jobs[1] -> timeout : 0);
*/
    // Make sure we have the proper amount of memory allocated
    if (decks[job].track_lengths != NULL)
      free(decks[job].track_lengths);

    decks[job].track_lengths = (int *)malloc(
      decks[job].total_tracks * sizeof(int));

    // Post a message for our user class
    AddMessage(TOTALTRACKS, job, jobs[job]->disc, decks[job].total_tracks);

    // Lock down the bus, we're taking control.
    track[job] = 1;

    // Request first track length
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;
    pkt -> msg[1] = CMD_GET_TRACK;
    pkt -> msg[3] = BCD(track[job] = 1);
    pkt -> size = 4;

    if ((bcd = DiscBCD(jobs[job] -> disc))) {
      pkt -> msg[2] = bcd;
    } else {
      pkt -> msg[2] = (unsigned char)(jobs[job] -> disc - 200);
      id += 0x03;
    }

    pkt -> msg[0] = id;
    AddOutgoing(pkt);

    jobs[job] -> state = 4;
    jobs[job] -> timeout = DEFAULT_TIMEOUT;
    return 1;
  }

  if (jobs[job] -> state == 4) {
    if (incoming[1] == RSP_GET_TRACK) {

      if (track[job] >= decks[job].total_tracks) {
        jobs[job] -> state = 5;
        track[job] = -1;
        return 1;
      }

      pkt = (struct packet *)calloc(sizeof(struct packet), 1);
      pkt -> jiffie = jiffie + 1;
      pkt -> msg[1] = CMD_GET_TRACK;
      pkt -> msg[3] = BCD(++track[job]);
      pkt -> size = 4;

      if ((bcd = DiscBCD(jobs[job] -> disc))) {
        pkt -> msg[2] = bcd;
      } else {
        pkt -> msg[2] = (unsigned char)(jobs[job] -> disc - 200);
        id += 0x03;
      }

      pkt -> msg[0] = id;
      AddOutgoing(pkt);

      jobs[job] -> timeout = DEFAULT_TIMEOUT;
      return 1;
    }
  }

  if (jobs[job] -> state == 5) {
    track[job] = 0;

    for (i = 0; i < decks[job].total_tracks; i++) {
      length = 2;

      for (j = 0; j < i; j++)
        length += decks[job].track_lengths[j];

      while (length > 0) {
        track[job] += length % 10;
        length /= 10;
      }
    }

    decks[job].cddb = (((track[job] % 0xFF) << 24) | 
                       ((decks[job].disc_length - 2) << 8) |
                       (decks[job].total_tracks));
    track[job] = -1;

    // Post a message for our user class, Just for now.
    // AddMessage(DISCCDDB, job, jobs[job] -> disc, 1);

    // Going from SLOT to FULL.
    switch (decks[job].contents[jobs[job] -> disc - 1].slot) {
      default:
        decks[job].contents[jobs[job] -> disc - 1].cddb   = 0;
        decks[job].contents[jobs[job] -> disc - 1].slot   = SLOT_UNKNOWN;
        decks[job].contents[jobs[job] -> disc - 1].status = SLOT_UNKNOWN;
        break;
      case SLOT_EMPTY:

        // Have to know where it got full.
        // Make this so it uses other colors!

        decks[job].contents[jobs[job] -> disc - 1].cddb   = decks[job].cddb;
        decks[job].contents[jobs[job] -> disc - 1].slot   = SLOT_FULL;
        decks[job].contents[jobs[job] -> disc - 1].status = SLOT_NOT_REMOTE;

        break;
      case SLOT_FULL:
        if (decks[job].contents[jobs[job]->disc-1].cddb==decks[job].cddb) {
          decks[job].contents[jobs[job] -> disc - 1].status = SLOT_MATCHED;
        } else {
          // Funky check for where it was matched, if at all.
          // Make this so it uses other colors!
          decks[job].contents[jobs[job] -> disc - 1].status = SLOT_NOT_REMOTE;
          decks[job].contents[jobs[job] -> disc - 1].cddb = decks[job].cddb;
        }

        decks[job].contents[jobs[job] -> disc - 1].cddb = decks[job].cddb;
        decks[job].contents[jobs[job] -> disc - 1].slot = SLOT_FULL;
        break;
      case SLOT_UNKNOWN:
        decks[job].contents[jobs[job] -> disc - 1].cddb   = decks[job].cddb;
        decks[job].contents[jobs[job] -> disc - 1].slot   = SLOT_FULL;
        decks[job].contents[jobs[job] -> disc - 1].status = SLOT_CORRECTED;
    }

    if ((contents = fopen("contents", "wb")) != NULL) {
      fwrite(decks[DECK_A].contents, sizeof(struct content), 400, contents);
      fwrite(decks[DECK_B].contents, sizeof(struct content), 400, contents);
      fclose(contents);
    }

    // Issue a stop, thus the disc only loads
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;
    pkt -> msg[0] = id;
    pkt -> msg[1] = CMD_STOP;
    pkt -> size = 2;
    AddOutgoing(pkt);

    jobs[job] -> state = 6;
    jobs[job] -> timeout = DEFAULT_TIMEOUT;
    return 1;
  }

  if ((jobs[job] -> state == 6) && (deck >= 2)) {

    // Post a message for our user class
    AddMessage(DISCLOADED, job, jobs[job] -> disc);

    // Remove the job now that the stop command is complete
    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
    }
  }

  return 0;
} /* LoadDiscProcess() */

int CDPlayer::SetDiscMemoProcess(int deck) {
  unsigned char id = CMD_DECK_A1, bcd;
  struct packet *pkt;
  int job = deck;

  // Valid set disc memo states:
  //   1 - Issuing set disc memo command to driver
  //   2 - getting the ok from the deck and cleaning up

  if (deck >= 2)
    job = DeckID(deck);

  if ((jobs[job] -> state > 2) || (jobs[job] -> state < 1) ||
      (jobs[job] -> timeout <= 0)) {

    if (jobs[job] != NULL) {

      if (jobs[job] -> ptr != NULL)
        free(jobs[job] -> ptr);

      free(jobs[job]);
      jobs[job] = NULL;
    }

    return 1;
  }

  if (job == 1)
    id = CMD_DECK_B1;

  if (jobs[job] -> state == 1) {
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;
    pkt -> msg[1] = CMD_SET_DISC_MEMO;
    strncpy(3 + (pkt -> msg), (char *)jobs[job] -> ptr, 13);
    pkt -> size = 16;

    // Specified disc.  Try to set the memo field. 
    if ((bcd = DiscBCD(jobs[job] -> disc))) {
      pkt -> msg[2] = bcd;
    } else {
      pkt -> msg[2] = (unsigned char)(jobs[job] -> disc - 200);
      id += 0x03;
    }

    pkt -> msg[0] = id;
    AddOutgoing(pkt);

    jobs[job] -> state = 2;
    return 1;
  }

  // Response from deck, deck = deck ID
  if ((jobs[job] -> state >= 2) && (deck >= 2)) {
    AddMessage(DISCMEMOSET, job, decks[job].disc, decks[job].track);

    // Remove the set disc memo command now that its complete
    if (jobs[job] != NULL) {

      if (jobs[job] -> ptr != NULL)
        free(jobs[job] -> ptr);

      free(jobs[job]);
      jobs[job] = NULL;
    }

    return 1;
  }

  return 0;
} /* SetDiscMemoProcess() */

int CDPlayer::SetGroupMemoProcess(int deck) { return 1; }
int CDPlayer::SetExtendedMemoProcess(int deck) { return 1; }
int CDPlayer::StandbyProcess(int deck) { return 1; }

int CDPlayer::ContentsProcess(int deck, unsigned char type) {
  static char loaded[2][52];
  unsigned char id = CMD_DECK_A1, bcd;
  struct packet *pkt;
  int offset = 0, decode = 0, job = deck;
  FILE *contents;

  // Valid contents states:
  //   1 - Queue traversel by 20 disc increments
  //   2 - Request bit packeted disc status
  //   3 - Set contents off status, pause first disc
  //   4 - Stop when first disc queued

  if (deck >= 2)
    job = DeckID(deck);

  // Invalid state: Job is corrupt purge it
  if ((jobs[job] -> state > 4) || (jobs[job] -> state < 1) ||
      (jobs[job] -> timeout <= 0)) {
    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
    }

    return 1;
  }

  if (job == 1)
    id = CMD_DECK_B1;

  // Force traverse the carosel 360
  if ((jobs[job] -> state == 1) && (deck < 2)) {
    memset(loaded[job], 0, 52);

    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;
    pkt -> msg[0] = id;
    pkt -> msg[1] = CMD_STOP;
    pkt -> size = 2;
    AddOutgoing(pkt);

    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 20;
    pkt -> msg[0] = id;
    pkt -> msg[1] = CMD_PAUSE_DISC;
    pkt -> msg[2] = 0x01;
    pkt -> size = 3;
    AddOutgoing(pkt);

    for (int i = 20; i < decks[job].capacity; i += 10) {
      pkt = (struct packet *)calloc(sizeof(struct packet), 1);
      pkt -> jiffie = jiffie + 20 + i;
      pkt -> msg[1] = CMD_PAUSE_DISC;
      pkt -> size = 3;

      if ((bcd = DiscBCD(i))) {
        pkt -> msg[2] = bcd;
        offset = 0x00;
      } else {
        pkt -> msg[2] = (unsigned char)(i - 200);
        offset = 0x03;
      }

      pkt -> msg[0] = id + offset;
      AddOutgoing(pkt);
    }

    jobs[job] -> timeout = DEFAULT_TIMEOUT * 20;
    jobs[job] -> state = 2;
    return 1;
  }

  if ((jobs[job] -> state == 2) && (deck >= 2) && (type == RSP_READY)) {
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 20;
    pkt -> msg[0] = id;
    pkt -> msg[1] = CMD_GET_LOADED_DISCS;
    pkt -> size = 2;
    AddOutgoing(pkt);

    jobs[job] -> timeout = DEFAULT_TIMEOUT * 3;
    jobs[job] -> state = 3;
  }

  if ((jobs[job] -> state == 3) && (deck >= 2)) {
    switch (type) {
      default:  break;
      case RSP_LOADED_DISCS_1:
        memcpy(loaded[job], incoming + 2, 13);

        if (decks[job].capacity <= 100)
          decode = 1;

        break;
      case RSP_LOADED_DISCS_2:
        memcpy(loaded[job] + 13, incoming + 2, 13);

        if (decks[job].capacity <= 200)
          decode = 1;

        break;
      case RSP_LOADED_DISCS_3:
        memcpy(loaded[job] + 26, incoming + 2, 13);

        if (decks[job].capacity <= 300)
          decode = 1;

        break;
      case RSP_LOADED_DISCS_4:
        memcpy(loaded[job] + 39, incoming + 2, 13);
        decode = 1;
    }

    if (decode) {
      for (int j = 0; j < 52; j++) {
        for (int k = 0; k < 8; k++) {

          if (j * 8 + k >= decks[job].capacity) {
            if ((contents = fopen("contents", "wb")) != NULL) {
              fwrite(decks[DECK_A].contents, sizeof(struct content),
                     400, contents);
              fwrite(decks[DECK_B].contents, sizeof(struct content),
                     400, contents);
              fclose(contents);
            }

            AddMessage(UPDATEDCONTENTS, job);
            jobs[job] -> state = 4;
            return 1;
          }

          // Going from CASE to FULL.
          if (*(loaded[job] + j) >> k & 0x01) {
            switch (decks[job].contents[j * 8 + k].slot) {
              default:
                decks[job].contents[j * 8 + k].slot   = SLOT_UNKNOWN;
                decks[job].contents[j * 8 + k].status = SLOT_UNKNOWN;
              case SLOT_UNKNOWN:
                decks[job].contents[j * 8 + k].slot   = SLOT_FULL;
                decks[job].contents[j * 8 + k].status = SLOT_UNKNOWN;
                 break;
              case SLOT_EMPTY:
                decks[job].contents[j * 8 + k].slot   = SLOT_FULL;
                decks[job].contents[j * 8 + k].status = SLOT_UNKNOWN;
                break;
              case SLOT_FULL:
                decks[job].contents[j * 8 + k].status = SLOT_PRESUMED;
            }
          } else {
            // Going from CASE to MISSING.
            decks[job].contents[j * 8 + k].cddb = 0;

            switch (decks[job].contents[j * 8 + k].slot) {
              default:
                decks[job].contents[j * 8 + k].slot   = SLOT_UNKNOWN;
                decks[job].contents[j * 8 + k].status = SLOT_UNKNOWN;
              case SLOT_UNKNOWN:
                decks[job].contents[j * 8 + k].slot   = SLOT_EMPTY;
                decks[job].contents[j * 8 + k].status = SLOT_CORRECTED;
                break;
              case SLOT_EMPTY:
                decks[job].contents[j * 8 + k].status = SLOT_MATCHED;
                break;
              case SLOT_FULL:
                decks[job].contents[j * 8 + k].slot   = SLOT_EMPTY;
                decks[job].contents[j * 8 + k].status = SLOT_CORRECTED;
            }          
          }
        }
      }
    }

    return 1;
  }

  // The jobs really done, we're just tidying up.
  if ((jobs[job] -> state == 4) && (deck < 2)) {
    pkt = (struct packet *)calloc(sizeof(struct packet), 1);
    pkt -> jiffie = jiffie + 1;
    pkt -> msg[0] = id;
    pkt -> msg[1] = CMD_STOP;
    pkt -> size = 2;

    AddOutgoing(pkt);

    if (jobs[job] != NULL) {
      free(jobs[job]);
      jobs[job] = NULL;
    }

    return 1;
  }

  return 0;
} /* ContentsProcess() */

#endif // CD_PLAYER_CLASS

#ifndef CD_PLAYER_CLASS_H
#define CD_PLAYER_CLASS_H

#include "ds/linkedlist.h"
#include "ds/string.h"

#include "std/cdplayer.h"
#include "std/disc.h"
#include "entry.h"

class CDPlayer {
  private:
    unsigned long jiffie;
    int jiffies_per_second;
    int slink;

    struct command *jobs[2];   // Current jobs for decks
    struct deck decks[2];  // Deck configurations

    LinkedList<struct command *> queue_A;  // Queued commands for deck A
    LinkedList<struct command *> queue_B;  // Queued commands for deck B

    LinkedList<struct packet *> outgoing;   // Packets bound for driver
    char incoming[17];                      // Packet pulled from driver

    LinkedList<struct message *> messages;  // Status/error messages

    int AddMessage(int, int, int=0, int=0, int=0, int=0, void * = NULL);
    int AddCommand(int, struct command *);
    int AddOutgoing(struct packet *);

    int ResetDeck(int);
    int ProcessIncoming(int);

    unsigned char DeckID(unsigned char);
    unsigned char BCD(int);
    int DEC(unsigned char);
    unsigned char DiscBCD(int);
    int DiscDEC(unsigned char, unsigned char);

    int PlayProcess(int);
    int PauseProcess(int);
    int StopProcess(int);
    int FastForwardProcess(int);
    int RewindProcess(int);
    int FadeInProcess(int);
    int FadeOutProcess(int);
    int LoadDiscProcess(int);
    int StartupProcess(int);
    int ShutdownProcess(int);
    int SetDiscMemoProcess(int);
    int SetGroupMemoProcess(int);
    int SetExtendedMemoProcess(int);
    int StandbyProcess(int);
    int ContentsProcess(int, unsigned char);

  public:
    CDPlayer(void);
    ~CDPlayer(void);

    int Setup(int);
    int Shutdown(void);
    int ProcessEvents(unsigned long);

    struct deck *QueryDeck(int);
    struct command *QueryJob(int);
    int StopJob(int);
    int FlushMessages(int = -1);
    struct message *QueryMessage();
    struct message *PeekMessage();

    char *QueryRunTime(int);
    char *QueryRemainTime(int);

    int SetContent(int, int, int, int, long);
    int SetWarnTime(int, int = DEFAULT_WARN_TIME);
    struct content *QueryContent(int, int);

    int Startup(void);

    int Play(int, int = DEFAULT_DISC, int = DEFAULT_TRACK,
             int = DEFAULT_MIN, int = DEFAULT_SEC, int = DEFAULT_FRAME);
    int Pause(int, int = DEFAULT_DISC, int = DEFAULT_TRACK,
              int = DEFAULT_MIN, int = DEFAULT_SEC, int = DEFAULT_FRAME);
    int Stop(int, int = DEFAULT_STOP_DELAY);
    int FastForward(int, int = DEFAULT_DECK, int = DEFAULT_NO_ARG);
    int Rewind(int, int = DEFAULT_DECK, int = DEFAULT_NO_ARG);

    int FadeIn(int, int = DEFAULT_FADE);
    int FadeOut(int, int = DEFAULT_FADE);

    int LoadDisc(int, int);
    int RemoteMode(int, int);
    int Standby(int, int);
    int Contents(int);

    int SetDiscMemo(int, int, char *);
    int GetDiscMemo(int, int, void *);
    int SetGroupMemo(int, int, void *);
    int GetGroupMemo(int, int, void *);
    int SetExtendedDiscMemo(int, int, int, void *);
    int GetExtendedDiscMemo(int, int, int, void *);

}; /* class CDPlayer() */


#endif // CD_PLAYER_CLASS_H

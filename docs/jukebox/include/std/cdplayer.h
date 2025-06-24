#ifndef CDPLAYER_H
#define CDPLAYER_H

#include "std/deck.h"

// Available decks
#define DECK_A			0
#define DECK_B			1

// Maximum packet size
#define MAX_PACKET_SIZE         16

// Possible deck states
#define STATE_UNAVAILABLE	0
#define STATE_STANDBY		1
#define STATE_PLAYING		2
#define STATE_PAUSED		3
#define STATE_STOPPED		4
#define STATE_FASTFORWARD	5
#define STATE_REWIND		6
#define STATE_FADINGIN		7
#define STATE_FADINGOUT		8
#define STATE_CUEING		9

// Default parameter values
#define DEFAULT_DECK            DECK_A
#define	DEFAULT_DISC		0
#define	DEFAULT_TRACK		1
#define	DEFAULT_MIN		0
#define	DEFAULT_SEC		0
#define	DEFAULT_FRAME		0
#define	DEFAULT_NO_ARG		0
#define	DEFAULT_FADE		5
#define	DEFAULT_WARN_TIME	29
#define DEFAULT_STOP_DELAY      0

// Active job timeout, in jiffies
#define DEFAULT_TIMEOUT		(jiffies_per_second * 3)

// Valid user commands

#define	PLAY			1
#define	STOP			2
#define	PAUSE			3
#define	FASTFORWARD		4
#define	REWIND			5
#define	FADEIN			6
#define FADEOUT			7
#define LOADDISC		8
#define	STARTUP			9
#define CDDB			10
#define DISCINFO		11
#define STATUS                  12
#define REMOTE			13  
#define SETDISCMEMO		14
#define SETGROUPMEMO		15
#define SETEXTENDEDMEMO		16
#define	STANDBY			17
#define CONTENTS		18

// Valid user message responses

#define	PLAYING			1
#define	STOPPED			2
#define	PAUSED			3
#define	FASTFORWARDING		4
#define	REWINDING		5
#define	CUEING			6
#define	FADINGIN		7
#define	FADINGOUT		8
#define DISCLOADED		9
#define STARTEDUP		10
#define	STANDINGBY		11
#define	REMOTEMODEON		12
#define	REMOTEMODEOFF		13
#define	NODISC			14
#define	TOTALTRACKS		15
#define	REMAINTIME		16
#define	PLAYTIME		17
#define	NOTRACK			18
#define	TIMERCORRECTION		19
#define	WARNTIME		20
#define	DOOROPEN		21
#define	DOORCLOSED		22
#define	DISCMEMOSET		23
#define	GROUPMEMOSET		24
#define	EXTENDEDDISCMEMOSET	25
#define	DECKMODEL		26
#define	DECKCAPACITY		27
#define UPDATEDCONTENTS		28
#define TRACKDONE		29
#define DISCCDDB		30
#define FADEDIN                 31
#define FADEDOUT                32
#define FADEINFAIL              33
#define FADEOUTFAIL             34

// Valid deck command control codes

#define CMD_DECK_A1		0x90
#define CMD_DECK_A2		0x93
#define CMD_DECK_B1		0x91
#define CMD_DECK_B2		0x94

#define CMD_PLAY		0x00
#define CMD_STOP		0x01
#define CMD_PAUSE		0x02
#define CMD_NEXT_TRACK		0x08
#define CMD_PREV_TRACK		0x09
#define CMD_FASTFORWARD		0x10
#define CMD_REWIND		0x11
#define CMD_FADEOUT		0x5E
#define CMD_FADEIN		0x5F
#define CMD_POWERON		0x2E
#define CMD_POWEROFF		0x2F
#define CMD_REMOTEON		0x20
#define CMD_REMOTEOFF		0x21
#define CMD_VERBOSE		0x25
#define CMD_BRIEF		0x26
#define CMD_PLAY_DISC		0x50
#define CMD_PAUSE_DISC		0x51
#define CMD_GET_DECK_MODEL	0x6A
#define CMD_GET_TRACKS		0x41
#define CMD_GET_DECK_STATUS	0x0F
#define CMD_GET_DECK_CAPACITY	0x22
#define CMD_GET_DISC		0x44
#define CMD_GET_TRACK		0x45
#define CMD_GET_DISC_MEMO	0x40
#define CMD_GET_GROUP_MEMO	0x42
#define CMD_GET_GROUP_DISCS	0x43
#define CMD_GET_LOADED_DISCS    0x72
#define CMD_SET_DISC_MEMO	0x80

// Valid deck response control codes

#define RSP_DECK_A1		0x98
#define RSP_DECK_A2		0x9B
#define RSP_DECK_B1		0x99
#define RSP_DECK_B2		0x9C

#define RSP_PLAYING		0x00
#define RSP_STOPPED		0x01
#define RSP_PAUSED		0x02
#define RSP_PLAYING_DISC	0x50
#define RSP_PLAYING_DISC_AT	0x51
#define RSP_DISPLAYING_DISC	0x52
#define RSP_POWERON		0x2E
#define RSP_POWEROFF		0x2F
#define RSP_TRAVERSING		0x06
#define RSP_READY		0x08
#define RSP_UNLOADING_DISC	0x54
#define RSP_LOADING_DISC	0x58
#define RSP_DECK_CAPACITY	0x61
#define RSP_DECK_MODEL		0x6A
#define RSP_DECK_STATUS		0x70
#define RSP_GET_DISC		0x60
#define RSP_GET_TRACK		0x62
#define RSP_GET_TRACKS		0x41
#define RSP_MEMO_SET		0x1F
#define RSP_MEMO_SET_1		0x44
#define RSP_MEMO_SET_2		0x45
#define RSP_GET_MEMO		0x40
#define RSP_GET_GROUP		0x42
#define RSP_29_REMAIN		0x0C
#define RSP_DOOR_OPEN		0x18
#define RSP_DOOR_CLOSED		0x83
#define RSP_NO_DISC		0x05
#define RSP_DISC_UNLOADED	0x14
#define RSP_TRACK_UNLOADED	0x15	
#define RSP_DUPLICATE_CMD	0x0E
#define RSP_INVALID_CMD		0x0F
#define RSP_LOADED_DISCS_1      0x72
#define RSP_LOADED_DISCS_2      0x73
#define RSP_LOADED_DISCS_3      0x74
#define RSP_LOADED_DISCS_4      0x75

// Bit packed deck play mode masks

#define CONTINUOUS_MASK		0x01
#define SHUFFLE_MASK		0x02
#define PROGRAM_MASK		0x04
#define REPEAT_MASK		0x08
#define TRACK_MASK		0x10
#define DISC_MASK		0x20
#define REMOTE_MASK		0x40

// Existing deck contents

#define SLOT_UNKNOWN            0
#define SLOT_EMPTY              1
#define SLOT_FULL               2

#define SLOT_MATCHED            1
#define SLOT_PRESUMED           2
#define SLOT_CORRECTED          3
#define SLOT_NOT_LOCAL          4
#define SLOT_NOT_REMOTE         5

// Response structure, passed back to control object

struct message {
  int id;     // Message type
  int deck;   // Relevent deck
  int disc;   // Relevent disc
  int track;  // Relevent track
  int min;    // Relevent minute
  int sec;    // Relevent second
  void *ptr;  // Other data
}; /* struct message */

// Pending command structure

struct packet {
  unsigned long jiffie;  // Send at jiffie count;  0 send ASAP
  char msg[16];          // Message to send to driver
  int size;		 // Length of packet
}; /* struct packet */

// Active command structure

struct command {
  unsigned long jiffie;  // Send at jiffie count; 0 send ASAP
  int timeout;           // Timeout duration, lets us recover
  int cmd;               // Command
  int disc;              // Disc
  int track;             // Track
  int min;               // Minutes
  int sec;               // Seconds
  int frame;             // Frames
  void *ptr;             // Pointer to extra data
  int state;             // Current command state
}; /* struct command */

#endif  // CDPLAYER_H

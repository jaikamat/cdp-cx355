#ifndef CDDB_H
#define CDDB_H

#define CDDB_SERVER_1       "us.cddb.com"     // Random, US
#define CDDB_SERVER_2       "in.us.cddb.com"  // Carmel, IN US
#define CDDB_SERVER_3       "ca.us.cddb.com"  // Berkley, CA US

#define CDDB_PORT           8880

#define BUFFER_SIZE         1024

#define SOCK_UNAVAILABLE    0
#define SOCK_CLOSED         1
#define SOCK_DISCONNECTING  2
#define SOCK_OPEN           3
#define SOCK_CONNECTING     4
#define SOCK_HANDSHAKING    5

#define SERV_MESSAGE        200
#define SERV_READY          201
#define SERV_DATA           210
#define SERV_DISCONNECT     230
#define SERV_ERR_MATCH      401
#define SERV_ERR            500
#define SERV_ERR_CAT        501
#define SERV_ERR_DUP        502
#define SERV_TIMEOUT        530

struct cddb {
  long id;
  char catagory[16];
  int tracks;
  long *offsets;
  int length;
}; /* struct cddb */

#endif // CDDB_H

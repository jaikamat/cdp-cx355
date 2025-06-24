// CD Database (CDDatabase) Client
// Suplimental GAL Protocol Object

#ifndef CD_DATABASE_CLASS
#define CD_DATABASE_CLASS

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include <netinet/in.h>

#include "protocol/cddb.h"

CDDatabase::CDDatabase(void) {
  host = (char *)malloc(strlen(CDDB_SERVER_1) + 1);

  strcpy(host, CDDB_SERVER_1);
  port = CDDB_PORT;

  status = SOCK_CLOSED;
  strcpy(status_str, "Disconnected.\n");
  skip_local = cat_size = handle = 0;
  catagories = NULL;

} /* CDDatabase() */

CDDatabase::CDDatabase(char *h, int p) {
  host = (char *)malloc(strlen(h) + 1);

  strcpy(host, h);
  port = p;
  status = SOCK_CLOSED;
  strcpy(status_str, "Disconnected.\n");
  skip_local = cat_size = handle = 0;
  catagories = NULL;

} /* CDDatabase() */

CDDatabase::~CDDatabase(void) {
  free(host);

  if (catagories != NULL)
    free(catagories);

} /* ~CDDatabase */

// Attempt socket connection
int CDDatabase::Connect(struct hostent *hp) {
  struct sockaddr_in sin;

  strcpy(status_str, "Attempting Connection.\n");

  bzero((char *)&sin, sizeof(sin));
  bcopy(hp -> h_addr, (char *)&sin.sin_addr, hp -> h_length);
  sin.sin_family = hp -> h_addrtype;
  sin.sin_port = htons(CDDB_PORT);

  if ((fd = socket(sin.sin_family, SOCK_STREAM, 0)) == -1) {
    perror("Socket");
    return (status = SOCK_UNAVAILABLE);
  }

  if (connect(fd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
    perror("Connect");
    return (status = SOCK_UNAVAILABLE);
  }

  return (status = SOCK_CONNECTING);
} /* Connect() */

// Connect by address
int CDDatabase::Connect(char *host_addr, int len) {
  struct hostent *hp;

  if ((hp = gethostbyaddr(host_addr, len, AF_INET)) == NULL)
    return (status = SOCK_UNAVAILABLE);

  return Connect(hp);
} /* Connect() */

// Connect by name
int CDDatabase::Connect(char *host_name) {
  struct hostent *hp;

  if ((hp = gethostbyname(host_name)) == NULL)
    return (status = SOCK_UNAVAILABLE);

  return Connect(hp);
} /* Connect() */

int CDDatabase::Disconnect(void) {

  if (!close(fd)) {
    strcpy(status_str, "Disconnected.\n");
    return (status = SOCK_CLOSED);
  }

  requests.ClearList();
  responses.ClearList();

  return (status = SOCK_UNAVAILABLE);
} /* Disconnect() */

int CDDatabase::Login(void) {
  char login[] = "cddb HELLO user Watt SLink-Jukebox 1.0\n";

  if (status != SOCK_CONNECTING)
    return 0;

  status = SOCK_HANDSHAKING;
  strcpy(status_str, "Logging in.\n");

  return send(fd, login, strlen(login), 0);
} /* Login() */

int CDDatabase::Catagories(void) {
  char cat[] = "cddb LSCAT\n";

  if (status != SOCK_OPEN)
    return 0;

  strcpy(status_str, "Requesting Catagories.\n");

  return send(fd, cat, strlen(cat), 0);
} /* Catagories() */

int CDDatabase::Setup(void) { 

  status = SOCK_CLOSED;
  cat_size = handle = 0;
  catagories = NULL;

  Connect(CDDB_SERVER_1);
  return 1;
} /* Setup() */

char *CDDatabase::Status(void) { return status_str; }
int CDDatabase::SocketStatus(void) { return status; }

int CDDatabase::ProcessEvents(unsigned long jiffie) {
  static char buf[BUFFER_SIZE], path[80];
  char str[BUFFER_SIZE];
  FILE *import;
  int i;
  long id, size;

  // There's no point, unable to allocate socket
  if (status == SOCK_UNAVAILABLE)
    return 0;

  if (status >= SOCK_OPEN) {
    bzero(buf, BUFFER_SIZE);

    if (recv(fd, buf, BUFFER_SIZE, 0) > 0)
      ProcessIncoming(buf);
  }

  if (requests.ListSize() > 0) {

    // Check local exported CDDB files
    fprintf(stdout, "REQUEST: %s %lX %d %d         ",
            requests.Index(0) -> cd.catagory,
            requests.Index(0) -> cd.id,
            skip_local, requests.Index(0) -> id);

    if (!skip_local) {
      id = requests.Index(0) -> cd.id;

      for (i = 0; i < cat_size; i++) {
        fprintf(stdout, "%d", i);
        sprintf(path, "cddb/%s/%lX", catagories + (i * 16), id);

        if ((import = fopen(path, "r")) != NULL) {
          bzero(str, BUFFER_SIZE);

          fseek(import, 0, SEEK_END);
          size = ftell(import);
          fseek(import, 0, SEEK_SET);
          fread(str, size, sizeof(char), import);

          fprintf(stdout, "***");
          BuildDisc(str);
          fprintf(stdout, "***");

          // Purge all other requests (BIG TIME KLUDGE)
          requests.ClearList();
          fclose(import);

          fprintf(stdout, "***");
        }
      }

      return 1;
    }

    if (status == SOCK_CLOSED)
      Connect(CDDB_SERVER_1);

    if ((!(requests.Index(0) -> ts))&&(status==SOCK_OPEN) && (cat_size))
      ProcessOutgoing(jiffie);

  } else {
    if ((status == SOCK_OPEN) && (cat_size))
      Disconnect();
  }

  return 1;
} /* ProcessEvents() */

int CDDatabase::ProcessIncoming(char *buf) {
  int code;

  if (!strlen(buf))
    return 0;

  sscanf((char *)buf, "%d%*s", &code);

  switch(code) {
    case SERV_MESSAGE:    ProcessMessage(buf);             break;
    case SERV_READY:      Login();                         break;
    case SERV_DATA:       ProcessMessage(buf);             break;
    case SERV_DISCONNECT: Disconnect();                    break;
    case SERV_ERR_MATCH:

      if (skip_local > 0)
        skip_local--;

      requests.DeleteFront();
      Disconnect();
      break;
    
    case SERV_ERR:        

      if (skip_local > 0)
        skip_local--;

      strcpy(status_str, "Error.\n");
      requests.DeleteFront();
      Disconnect();
      break;

    case SERV_ERR_CAT:

      if (skip_local > 0)
        skip_local--;

      requests.DeleteFront();
      break;

    case SERV_ERR_DUP:    Disconnect();                    break;
    case SERV_TIMEOUT:    Connect(CDDB_SERVER_1);
  }

  return 1;
} /* ProcessIncoming() */

int CDDatabase::ProcessOutgoing(unsigned long jiffie) {
  struct request *req;
  char packet[BUFFER_SIZE], offset[8];
  int i;

  req = requests.Index(0);
  req -> ts = jiffie;

  if (!strlen(req -> cd.catagory)) {
    sprintf(packet, "cddb QUERY %-8lx %d ", req -> cd.id, 
            req -> cd.tracks);

    for (i = 0; i < req -> cd.tracks; i++) {
      sprintf(offset, "%ld ", req -> cd.offsets[i]);
      strcat(packet, offset);
    }

    sprintf(offset, "%d\n", req -> cd.length);
    strcat(packet, offset);
    strcpy(status_str, "Requesting Catagory.\n");

    send(fd, packet, strlen(packet), 0);
  } else {
    sprintf(packet, "cddb READ %s %-8lx\n", req -> cd.catagory,
            req -> cd.id);
    strcpy(status_str, "Requesting CD Information.\n");

    send(fd, packet, strlen(packet), 0);    
  }

  return 1;
} /* ProcessOutgoing() */

int CDDatabase::ProcessMessage(char *str) {
  struct request *req;
  char key[16], *ptr;
  int code;

  ptr = str + 4;
  sscanf(str, "%d %s %*s", &code, key);

  if (code == SERV_MESSAGE)
    if (strstr(ptr, "Hello and welcome") != NULL) {
      status = SOCK_OPEN;
      strcpy(status_str, "Logged in.\n");

      if (!cat_size)
        Catagories();

    } else {
      if (CatagoryExists(key)) {
        req = requests.Index(0);

        strcpy(req -> cd.catagory, key); 
        req -> ts = 0;
      } else {

        if (skip_local > 0)
          skip_local--;

        requests.DeleteFront();
      }
    }
  else
    if (code == SERV_DATA)
      if (strstr(ptr, "category list") != NULL)
        BuildCatagories(str);
      else
        if (strstr(ptr, "CD database entry") != NULL)
          BuildDisc(str);
    else
      return 0;

  return 1;
} /* ProcessMessage() */

int CDDatabase::BuildDisc(char *str) {
  struct disc *dsc;
  struct response *rsp;

  if (str == NULL) {
    rsp = (struct response *)malloc(sizeof(struct response));
    rsp -> id = requests.Index(0) -> id;
    rsp -> cd = NULL;
  } else {

    if ((dsc = Import(str)) == NULL)
      return 0;

    // Right now if you reID a disc locally it doesn't know it's catagory
//    strcpy(dsc -> indexs[index].catagory,
//           requests.Index(0) -> cd.catagory);

    rsp = (struct response *)malloc(sizeof(struct response));
    rsp -> id = requests.Index(0) -> id;
    rsp -> cd = dsc;
  }

  if (skip_local > 0)
    skip_local--;

  requests.DeleteFront();
  responses.InsertRear(rsp);
  Disconnect();

  return 1;
} /* BuildDisc() */

int CDDatabase::CatagoryExists(char *str) {
  int i;

  for (i = 0; i < cat_size; i++)
    if (!strcmp(str, catagories + (i * 12)))
      return 1;

  return 0;
} /* CatagoryExists() */

int CDDatabase::BuildCatagories(char *str) {
  char buf[80], cat[16], *ptr = str;
  int i, size = strlen(str);

  if (catagories != NULL)
    return 0;

  cat_size = -2;
  strcpy(status_str, "Building Catagories.\n");

  do {
    bzero(buf, 80);
    str += ((char *)memccpy(buf, str, 10, 80) - buf);
    cat_size++;
  } while (str < (ptr + size));

  if (cat_size > 0) {
    catagories = (char *)malloc(cat_size * 16);
    ptr += ((char *)memccpy(buf, ptr, 10, 80) - buf);

    for (i = 0; i < cat_size; i++) {
      bzero(buf, 80);
      ptr += ((char *)memccpy(buf, ptr, 10, 80) - buf);
      sscanf(buf, "%s\n", cat);
      strcpy(catagories + (i * 16), cat);
    }

    return cat_size;
  }

  return (cat_size = 0);
} /* BuildCatagories() */

// Valid CDDatabase requests require these fields minimum:
// A) cddb.id, cddb.catagory; OR
// B) cddb.id, cddb.tracks, cddb.*offsets, cddb.length

int CDDatabase::Request(struct cddb pcddb) {
  struct request *req;
  struct cddb *cddb1;
  FILE *import;
  char path[80];
  int i;

  if (!(pcddb.id))
    return -1;

  req = (struct request *)malloc(sizeof(struct request));

  if ((pcddb.id) && (strlen(pcddb.catagory) > 0)) {
    pcddb.tracks = 0;
    pcddb.offsets = NULL;
    pcddb.length = 0;
  } else {
    if ((pcddb.id) && (pcddb.tracks > 0) && 
        (pcddb.offsets != NULL) && (pcddb.length > 0)) {
      pcddb.catagory[0] = '\0';
    } else {

      skip_local = cat_size;

      for (i = 0; i < cat_size; i++) {
        sprintf(path, "cddb/%s/%lX", catagories + (i * 16), pcddb.id);

        if ((import = fopen(path, "r")) != NULL) {
          fclose(import);
          skip_local = 0;
          break;
        }
      }

      for (i = 0; i < cat_size; i++) {
        cddb1 = (struct cddb *)malloc(sizeof(struct cddb));
        cddb1 -> id = pcddb.id;
        strcpy(cddb1 -> catagory, catagories + (i * 16));
        Request(*cddb1);
      }

      // Free it?
      return 0;
    }
  }

  req -> id = handle++;
  req -> ts = 0;
  req -> cd = pcddb;

  requests.InsertRear(req);

  return req -> id;
} /* Request() */

int CDDatabase::Responses(void) { return responses.ListSize(); }

int CDDatabase::Response(struct disc *&dsc) {
  struct response *rsp;
  int id;

  if (!responses.ListSize())
    return -1;

  rsp = responses.DeleteFront();
  memcpy(dsc, rsp -> cd, sizeof(struct disc));
  id = rsp -> id;

  free(rsp);

  return id;
} /* Response() */

struct disc *CDDatabase::Import(char *str) {
  struct disc *dsc;
  char buf[80], *brk, *strt, *nl, *ptr = str;
  int index, offset, size = strlen(str), tracks = -1;
  FILE *export;

  fprintf(stdout, "%s", "FOO!");

  fprintf(stdout, "%s", str);

  do {
    bzero(buf, 80);
    str += ((char *)memccpy(buf, str, 10, 80) - buf);

    if (!strncmp(buf, "TTITLE", 6))
      sscanf(buf, "TTITLE%d=%*s", &tracks);

  } while (str < (ptr + size));

  if (tracks == -1)
    return NULL;

  tracks++;
  str = ptr;

  fprintf(stdout, "%s", "FOO");

  // Remember the client MUST free the structure once they're done
  dsc = (struct disc *)malloc(sizeof(struct disc));
  dsc -> indexs = (struct track *)malloc(sizeof(struct track) * tracks);
  dsc -> id.offsets = (long *)malloc(sizeof(long) * tracks);

  bzero(dsc -> id.catagory, 16);

  if (requests.ListSize() > 0)
    strcpy(dsc -> id.catagory, requests.Index(0) -> cd.catagory);

  bzero(dsc -> title, 40);
  bzero(dsc -> artist, 40);
  dsc -> id.tracks = tracks;
  dsc -> tracks = tracks;

  do {
    bzero(buf, 80);
    str += ((char *)memccpy(buf, str, 10, 80) - buf);

    if (!strncmp(buf, "# Track frame offsets", 21)) {
      for (offset = 0; offset < tracks; offset++) {
        str += ((char *)memccpy(buf, str, 10, 80) - buf);
        sscanf(buf, "#       %ld\n", &(dsc -> id.offsets[offset]));
      }

      continue;
    } 

    if (!strncmp(buf, "# Disc length", 13)) {
      sscanf(buf, "# Disc length: %d", &(dsc -> id.length));
      continue;
    }

    if (!strncmp(buf, "DISCID=", 7)) {
      sscanf(buf, "DISCID=%8lx", &(dsc -> id.id));
      continue;
    }

    if (!strncmp(buf, "DTITLE=", 7)) {
      brk = strchr(buf + 7, '/') - 1;
      nl = strchr(buf + 7, 10);
      brk[0] = brk[1] = nl[0] = '\0';
      strncpy(dsc -> artist, buf + 7, 39);
      strncpy(dsc -> title, brk + 2, 39);
      continue;
    }

    if (!strncmp(buf, "TTITLE", 6)) {
      sscanf(buf, "TTITLE%d=", &index);
      strt = (strchr(buf, '=') + 1);

      bzero(nl = strchr(strt, 10), 1);
      bzero(dsc -> indexs[index].title, 40);
      bzero(dsc -> indexs[index].artist, 40);

      dsc -> indexs[index].track = index + 1;

      if (requests.ListSize() > 0)
        strcpy(dsc -> indexs[index].catagory,
               requests.Index(0) -> cd.catagory);

      if ((brk = strchr(strt, '/')) == NULL) {
        strncpy(dsc -> indexs[index].artist, dsc -> artist, 39);
        strncpy(dsc -> indexs[index].title, strt, 39);
      } else {
        bzero(brk - 1, 2);
        strncpy(dsc -> indexs[index].artist, strt, 39);
        strncpy(dsc -> indexs[index].title, brk + 1, 39);
      }
    }
  } while (str < (ptr + size));

  fprintf(stdout, "%s", "FOO");

  if (dsc -> id.catagory[0] != '0')
    sprintf(buf, "cddb/%s/%lX", dsc -> id.catagory, dsc -> id.id);

  fprintf(stdout, "%s", buf);

  // Store CDDB Export Locally
  if ((export = fopen(buf, "w+")) != NULL) {
    fwrite(strchr(ptr, '#'), size, sizeof(char), export);
    fclose(export);
  }

  return dsc;
} /* Import() */

#endif // CD_DATABASE_CLASS

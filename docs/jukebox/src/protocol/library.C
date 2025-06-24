#ifndef LIBRARY_CLASS
#define LIBRARY_CLASS

#define LIBRARY_FILE  "library"
#define TEMP_FILE     "library.temp"

#include <jukebox.h>
#include <protocol/library.h>
#include <protocol/cdplayer.h>

int Library::SaveDisc(int lib, struct disc *disc) {
  struct cddb cddb;
  int i, size;

  if (lib < 0)
    return 0;

  lseek(lib, 0, SEEK_END);
  cddb = disc -> id;
  size = (96 * sizeof(char)) + (3 * sizeof(int)) + sizeof(long) +
         (cddb.tracks * sizeof(long)) +
         (disc -> tracks * sizeof(struct track));

  // Write out the CDDB structure
  write(lib, &cddb.id, sizeof(long));
  write(lib, &cddb.catagory, sizeof(char) * 16);
  write(lib, &cddb.tracks, sizeof(int));

  write(lib, cddb.offsets, cddb.tracks * sizeof(long));
  write(lib, &cddb.length, sizeof(int));

  // Write out the disc structure
  write(lib, disc -> title, sizeof(char) * 40);
  write(lib, disc -> artist, sizeof(char) * 40);
  write(lib, &disc -> tracks, sizeof(int));

  // Write out all track structures
  for (i = 0; i < disc -> tracks; i++) {
    write(lib, disc -> indexs[i].title, sizeof(char) * 40);
    write(lib, disc -> indexs[i].artist, sizeof(char) * 40);
    write(lib, disc -> indexs[i].catagory, sizeof(char) * 16);
    write(lib, &disc -> indexs[i].bmp_in, sizeof(int));
    write(lib, &disc -> indexs[i].bmp_out, sizeof(int));
    write(lib, &disc -> indexs[i].intro_time, sizeof(int));
    write(lib, &disc -> indexs[i].intro_transition, sizeof(int));
    write(lib, &disc -> indexs[i].outro_time, sizeof(int));
    write(lib, &disc -> indexs[i].outro_transition, sizeof(int));
    write(lib, &disc -> indexs[i].deck, sizeof(int));
    write(lib, &disc -> indexs[i].disc, sizeof(int));
    write(lib, &disc -> indexs[i].trk, sizeof(int));
    write(lib, &disc -> indexs[i].length, sizeof(int));
    write(lib, &disc -> indexs[i].script, sizeof(int));
  }
  
  return size;
} /* SaveDisc() */

struct disc *Library::RetrieveDisc(long offset, long *next) {
  struct disc *disc;
  struct stat stats;
  static long last_offset = 0;
  int i;

  if (library < 3) {
    PostError("Error: Library == NULL");
    return NULL;
  }

  if (fstat(library, &stats) == -1) {
    PostError(strerror(errno));
    return NULL;
  }

  if (offset >= 0)
    last_offset = offset;

  if (last_offset >= stats.st_size) {
    last_offset = 0;
    return NULL;
  }

  lseek(library, last_offset, SEEK_SET);
  disc = (struct disc *)calloc(sizeof(struct disc), 1);

  // Read the CDDB structure
  read(library, &disc -> id.id, sizeof(long));
  read(library, &disc -> id.catagory, sizeof(char) * 16);
  read(library, &disc -> id.tracks, sizeof(int));
  disc -> id.offsets = (long *)calloc(disc->id.tracks * sizeof(long), 1);

  read(library, disc -> id.offsets, sizeof(long) * disc -> id.tracks);
  read(library, &disc -> id.length, sizeof(int));

  // Read the disc structure
  read(library, disc -> title, sizeof(char) * 40);
  read(library, disc -> artist, sizeof(char) * 40);
  read(library, &disc -> tracks, sizeof(int));
  disc->indexs=(struct track *)calloc(disc->tracks*sizeof(struct track),1);
  
  // Read all track structures
  for (i = 0; i < disc -> tracks; i++) {
    read(library, disc -> indexs[i].title, sizeof(char) * 40);
    read(library, disc -> indexs[i].artist, sizeof(char) * 40);
    read(library, disc -> indexs[i].catagory, sizeof(char) * 16);
    read(library, &disc -> indexs[i].bmp_in, sizeof(int));
    read(library, &disc -> indexs[i].bmp_out, sizeof(int));
    read(library, &disc -> indexs[i].intro_time, sizeof(int));
    read(library, &disc -> indexs[i].intro_transition, sizeof(int));
    read(library, &disc -> indexs[i].outro_time, sizeof(int));
    read(library, &disc -> indexs[i].outro_transition, sizeof(int));
    read(library, &disc -> indexs[i].deck, sizeof(int));
    read(library, &disc -> indexs[i].disc, sizeof(int));
    read(library, &disc -> indexs[i].trk, sizeof(int));
    read(library, &disc -> indexs[i].length, sizeof(int));
    read(library, &disc -> indexs[i].script, sizeof(int));
  }

  last_offset += ((96 * sizeof(char)) + (3 * sizeof(int)) + sizeof(long) +
         (disc -> id.tracks * sizeof(long)) +
         (disc -> tracks * sizeof(struct track)));

  if (next != NULL)
    *next = last_offset;

  return disc;
} /* RetrieveDisc() */

// Not fast, but it'll gaurentee a match; We use the CDDB id to locate
// all possible disc matchs, then load each from disc and compare the
// full expanded versions byte by byte.  A very strict comparison.
int Library::DiscExists(struct disc *disc1) {
  struct disc *disc2;
  struct cddb cddb;
  CDDBEntry data;
  int size;

  if ((library < 0) || (disc1 == NULL))
    return 0;

  data.SetCDDB(disc1 -> id.id);

  if (cddbs.Find(data)) {
    for (int i = 0; i < data.OffsetSize(); i++) {
      if (!(disc2 = RetrieveDisc(data.QueryOffset(i), NULL)))
        continue;

      // This might be it, but it's already flaged to be purged
      if ((unsigned long)(disc2 -> id.id) == 0xFFFFFFFF) {
        free(disc2 -> id.offsets);
        free(disc2 -> indexs);
        free(disc2);
        continue;
      }

      cddb = disc2 -> id;
      size = (96 * sizeof(char)) + (3 * sizeof(int)) + sizeof(long) +
             (cddb.tracks * sizeof(long)) +
             (disc2 -> tracks * sizeof(struct track));

      if (!memcmp(disc1, disc2, size)) {
        free(disc2 -> id.offsets);
        free(disc2 -> indexs);
        free(disc2);
        return 1;
      }

      free(disc2 -> id.offsets);
      free(disc2 -> indexs);
      free(disc2);
    }

    return 1;
  }

  return 0;
} /* DiscExists() */

int Library::Setup(CDPlayer *cdp) {

  if (cdp != NULL)
    cd_player = cdp;

  if (!Open())
    return 0;

//  Pack();  // Pack the database to make sure things stay under control.
  BuildIndecies();

  return 1;
} /* Setup() */

int Library::Open(void) {

  if ((library = open(LIBRARY_FILE, O_RDWR | O_CREAT |
       O_APPEND | O_SYNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) == -1)
    return 0;

  lseek(library, 0, SEEK_SET);
  return 1;
} /* Open() */

int Library::Close(void) {
  int ret = close(library);
  library = -1; 
  
  return ret;
} /* Close() */

int Library::Purge(void) {

  Close();
  remove(LIBRARY_FILE);
  titles.ClearList();
  artists.ClearList();

  return 1;
} /* Purge() */

int Library::Pack(void) {
  struct disc *disc;
  int fd, size = 0;
  long this_offset = 0, next_offset;
  
  if (library < 0)
    return 0;

  if ((fd = open(TEMP_FILE, O_WRONLY | O_CREAT | O_TRUNC | O_SYNC,
       S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP)) == -1)
    return 0;

  lseek(library, 0, SEEK_SET);

  while (1) {
    
    if (!(disc = RetrieveDisc(this_offset, &next_offset)))
      break;

    if ((unsigned long)(disc -> id.id) == 0xFFFFFFFF)
      size++;
    else
      SaveDisc(fd, disc);

    this_offset = next_offset;
    free(disc -> id.offsets);
    free(disc -> indexs);
    free(disc);
  }

  Close();
  close(fd);

  remove(LIBRARY_FILE);
  rename(TEMP_FILE, LIBRARY_FILE);

  Setup();

  return size;  // Entries removed
} /* Pack() */

int Library::BuildIndecies(void) {
  BuildCDDBIndex(1);

  fprintf(stdout, "\nBuilding Indecies:");
  fprintf(stdout, "\n  Deck/Disc/Track: ");  BuildDeckDiscTrackIndex();
  fprintf(stdout, "\n  Title: ");            BuildTitleIndex();
  fprintf(stdout, "\n  Artist: ");           BuildArtistIndex();
//  fprintf(stdout, "\n  Keyword: ");          BuildKeywordIndex();
  fprintf(stdout, "\n");

  return 1;
} /* BuildIndecies() */

int Library::BuildCDDBIndex(int flag) {
  struct disc *disc;
  long this_offset = 0, next_offset;
  int total = 0, size = 0;

  while (1) {
    if ((disc = RetrieveDisc(this_offset, &next_offset)) == NULL)
      break;

    if ((unsigned long)(disc -> id.id) != 0xFFFFFFFF)
      total += disc -> tracks;

    this_offset = next_offset;
    free(disc -> id.offsets);
    free(disc -> indexs);
    free(disc);
  }

  if (!flag)
    fprintf(stdout, SAVE_LOC "(%d/%d)" LOAD_LOC, size, total);

  this_offset = 0;
  cddbs.ClearList();

  while (1) {

    if ((disc = RetrieveDisc(this_offset, &next_offset)) == NULL)
      break;

    if ((unsigned long)(disc -> id.id) == 0xFFFFFFFF) {
      free(disc -> id.offsets);
      free(disc -> indexs);
      free(disc);
      continue;
    }

    if ((!flag) && (!(size % 10)))
      fprintf(stdout, SAVE_LOC "(%d/%d)" LOAD_LOC, size, total);

    CDDBEntry data(disc -> id.id, this_offset);

    if (cddbs.Find(data)) { // Entry exists; add the next reference
      cddbs.GetCurrentData(data);
      data.AddOffset(this_offset);
    } else {                // New entry; stuff it in
      cddbs.Insert(data);
    }

    this_offset = next_offset;
    free(disc -> id.offsets);
    free(disc -> indexs);
    free(disc);
  }

  if (!flag)
    fprintf(stdout, SAVE_LOC "(%d/%d)" LOAD_LOC, size, total);

  return size;
} /* BuildCDDBIndex() */

// This only reflects the existing contents of the deck
// according to the CD Player class's most recent rebuild.
int Library::BuildDeckDiscTrackIndex(int flag) {
  struct disc *disc;
  struct content *contents;
  long this_offset = 0, next_offset;
  int fd, total = 0, size = 0;
  char str[100], artist[40], title[40];

  while (1) {
    if ((disc = RetrieveDisc(this_offset, &next_offset)) == NULL)
      break;

    if ((unsigned long)(disc -> id.id) != 0xFFFFFFFF)
      total += disc -> tracks;

    this_offset = next_offset;
    free(disc -> id.offsets);
    free(disc -> indexs);
    free(disc);
  }

  if (!flag)
    fprintf(stdout, SAVE_LOC "(%d/%d)" LOAD_LOC, size, total);

  this_offset = 0;
  ddt.ClearList();
  fd = open("ddt-index.txt", O_WRONLY | O_CREAT | O_SYNC,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

  while (1) {

    if ((disc = RetrieveDisc(this_offset, &next_offset)) == NULL)
      break;

    if ((unsigned long)(disc -> id.id) == 0xFFFFFFFF) {
      free(disc -> id.offsets);
      free(disc -> indexs);
      free(disc);
      continue;
    }

    for (int i = 0; i < disc -> tracks; i++) {
      size++;

      if ((!flag) && (!(size % 10)))
        fprintf(stdout, SAVE_LOC "(%d/%d)" LOAD_LOC, size, total);

      contents = cd_player -> QueryContent(disc -> indexs[i].deck,
                                           disc -> indexs[i].disc);

      // Only put the confirmed existing contents in this tree.
      if (contents -> cddb == disc -> id.id) {
        DeckDiscTrackEntry data(disc -> indexs[i].artist,
                                disc -> indexs[i].title,
                                disc -> indexs[i].deck,
                                disc -> indexs[i].disc, i + 1, this_offset, 1);
        ddt.Insert(data);

        if (fd > 0) {
          memset(artist, 0, 34);
          memset(title, 0, 34);
          strncpy(artist, disc -> indexs[i].artist, 33);
          strncpy(title, disc -> indexs[i].title, 33);
          sprintf(str, "%c %3d %2d %-34s %-34s\n",disc -> indexs[i].deck+0x41,
                  disc -> indexs[i].disc, i + 1, title, artist);
          write(fd, str,  strlen(str));
        }
      }
    }

    this_offset = next_offset;
    free(disc -> id.offsets);
    free(disc -> indexs);
    free(disc);
  }

  if (fd > 0)
    close(fd);

  if (!flag)
    fprintf(stdout, SAVE_LOC "(%d/%d)" LOAD_LOC, size, total);

  return size;
} /* BuildDeckDiscTrackIndex() */

int Library::BuildTitleIndex(int flag) {
  TitleEntry entry;
  struct disc *disc;
  struct content *contents;
  long this_offset = 0, next_offset;
  int fd, total = 0, size = 0, cur = 0;
  char str[100], artist[40], title[40];

  while (1) {
    if ((disc = RetrieveDisc(this_offset, &next_offset)) == NULL)
      break;

    if ((unsigned long)(disc -> id.id) != 0xFFFFFFFF)
      total += disc -> tracks;

    this_offset = next_offset;
    free(disc -> id.offsets);
    free(disc -> indexs);
    free(disc);
  }

  if (!flag)
    fprintf(stdout, SAVE_LOC "(%d/%d)" LOAD_LOC, size, total);

  this_offset = 0;
  titles.ClearList();    
  fd = open("title-index.txt", O_WRONLY | O_CREAT | O_SYNC,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

  while (1) {

    if ((disc = RetrieveDisc(this_offset, &next_offset)) == NULL)
      break;

    if ((unsigned long)(disc -> id.id) == 0xFFFFFFFF) {
      free(disc -> id.offsets);
      free(disc -> indexs);
      free(disc);
      continue;
    }

    for (int i = 0; i < disc -> tracks; i++) {
      size++;

      if ((!flag) && (!(size % 10)))
        fprintf(stdout, SAVE_LOC "(%d/%d)" LOAD_LOC, size, total);

      contents = cd_player -> QueryContent(disc -> indexs[i].deck,
                                           disc -> indexs[i].disc);
      TitleEntry data(disc -> indexs[i].artist, disc -> indexs[i].title,
                      disc -> indexs[i].deck, disc -> indexs[i].disc,
                      i + 1, this_offset, (unsigned long)contents -> cddb ==
                      (unsigned long)disc -> id.id);
      titles.Insert(data);
    }

    this_offset = next_offset;
    free(disc -> id.offsets);
    free(disc -> indexs);
    free(disc);
  }

  if (!flag)
    fprintf(stdout, SAVE_LOC "(%d/%d)" LOAD_LOC, size, total);

  for (int j = 0; j < 26; j++)
    letters[0][j] = -1;

  // Build first letter tree offsets
  for (int j = 0; j < size; j++) {
    titles.GetData(j, entry, 1);
    cur = toupper(entry.QueryTitle()[0]);

    if (fd > 0) {
      memset(artist, 0, 34);
      memset(title, 0, 34);
      strncpy(artist, entry.QueryArtist(), 33);
      strncpy(title, entry.QueryTitle(), 33);
      sprintf(str, "%c %3d %2d %-34s %-34s\n", entry.QueryDeck() + 0x41,
              entry.QueryDisc(), entry.QueryTrack(), title, artist);
      write(fd, str, strlen(str));
    }

    if ((cur >= 'A') && (cur <= 'Z'))
      if (letters[0][cur - 0x41] == -1)
        letters[0][cur - 0x41] = j;
  }

  if (fd > 0)
    close(fd);

  return size;
} /* BuildTitleIndex() */

int Library::BuildArtistIndex(int flag) {
  ArtistEntry entry;
  struct disc *disc;
  struct content *contents;
  long this_offset = 0, next_offset;
  int fd, total = 0, size = 0, cur = 0;
  char str[100], artist[40], title[40];

  while (1) {
    if ((disc = RetrieveDisc(this_offset, &next_offset)) == NULL)
      break;

    if ((unsigned long)(disc -> id.id) != 0xFFFFFFFF)
      total += disc -> tracks;

    this_offset = next_offset;
    free(disc -> id.offsets);
    free(disc -> indexs);
    free(disc);
  }

  if (!flag)
    fprintf(stdout, SAVE_LOC "(%d/%d)" LOAD_LOC, size, total);

  this_offset = 0;
  artists.ClearList();    
  fd = open("artist-index.txt", O_WRONLY | O_CREAT | O_SYNC,
            S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);

  while (1) {

    if ((disc = RetrieveDisc(this_offset, &next_offset)) == NULL)
      break;

    if ((unsigned long)(disc -> id.id) == 0xFFFFFFFF) {
      free(disc -> id.offsets);
      free(disc -> indexs);
      free(disc);
      continue;
    }

    for (int i = 0; i < disc -> tracks; i++) {
      size++;

      if ((!flag) && (!(size % 10)))
        fprintf(stdout, SAVE_LOC "(%d/%d)" LOAD_LOC, size, total);

      contents = cd_player -> QueryContent(disc -> indexs[i].deck,
                                           disc -> indexs[i].disc);
      ArtistEntry data(disc -> indexs[i].artist, disc -> indexs[i].title,
                       disc -> indexs[i].deck, disc -> indexs[i].disc,
                       i + 1, this_offset, (unsigned long)contents -> cddb ==
                       (unsigned long)disc -> id.id);
      artists.Insert(data);
    }

    this_offset = next_offset;
    free(disc -> id.offsets);
    free(disc -> indexs);
    free(disc);
  }

  if (!flag)
    fprintf(stdout, SAVE_LOC "(%d/%d)" LOAD_LOC, size, total);

  for (int j = 0; j < 26; j++)
    letters[1][j] = -1;

  // Build first letter tree offsets
  for (int j = 0; j < size; j++) {
    artists.GetData(j, entry, 1);
    cur = toupper(entry.QueryArtist()[0]);

    if (fd > 0) {
      memset(artist, 0, 34);
      memset(title, 0, 34);
      strncpy(artist, entry.QueryArtist(), 33);
      strncpy(title, entry.QueryTitle(), 33);
      sprintf(str, "%c %3d %2d %-34s %-34s\n", entry.QueryDeck() + 0x41,
              entry.QueryDisc(), entry.QueryTrack(), title, artist);
      write(fd, str, strlen(str));
    }

    if ((cur >= 'A') && (cur <= 'Z'))
      if (letters[1][cur - 0x41] == -1)
        letters[1][cur - 0x41] = j;
  }

  if (fd > 0)
    close(fd);

  return size;
} /* BuildArtistIndex() */

int Library::QueryLetter(char letter, int flag) {
  return letters[flag][toupper(letter) - 0x41];
} /* QueryLetter() */

int Library::AddDisc(struct disc *disc) {

  if (DiscExists(disc))
    return 0;

  return SaveDisc(library, disc);
} /* AddDisc() */

int Library::RemoveDisc(long offset) {
  unsigned long flag = 0xFFFFFFFF;

  // Using 0xFFFFFFFF as the first 32 bits (usually the CDDB id)
  // flags the entry to be purged.

  if (offset < 0)
    return 0;

  lseek(library, offset, SEEK_SET);
  write(library, &flag, sizeof(long));

  return 1;
} /* RemoveDisc() */

// Using the existing index of the deck contents.
struct track *Library::QueryTrack(int deck, int disc, int track) {
  DeckDiscTrackEntry data("", "", deck, disc, track, 0, 0);
  struct disc *dsc;
  struct track *trk;
  char buf[80];

  if (!ddt.Find(data)) {
    PostError("Unable to find entry");
    return NULL;
  }

  if ((dsc = RetrieveDisc(data.QueryOffset(), NULL)) == NULL) {
    sprintf(buf, "Unable to retrieve disc at offset %ld", data.QueryOffset());
    PostError(buf);
    return NULL;
  }

  trk = (struct track *)malloc(sizeof(struct track));
  memcpy(trk, &(dsc -> indexs[track - 1]), sizeof(struct track));

  sprintf(buf, "%s - %s", dsc -> indexs[track - 1].artist,
          dsc -> indexs[track - 1].title);
  PostError(buf);

  sprintf(buf, "%s - %s", trk -> artist, trk -> title);
  PostError(buf);

  free(dsc -> id.offsets);
  free(dsc -> indexs);
  free(dsc);

  PostError("QueryTrack Done");

  return trk;
} /* QueryTrack() */

// Returns a list of tracks that match the partial/full string.
LinkedList<struct track *> Library::QueryTrackTitle(char *title) {
  LinkedList<struct track *> tracks;

  return tracks;
} /* QueryTrackTitle() */

// Returns a list of tracks that match the partial/full string.
LinkedList<struct track *> Library::QueryTrackArtist(char *artist) {
  LinkedList<struct track *> tracks;

  return tracks;
} /* QueryTrackArtist() */

#endif // LIBRARY_CLASS

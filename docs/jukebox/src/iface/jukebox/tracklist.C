#ifndef TRACK_LIST_BOX
#define TRACK_LIST_BOX

#include <iface/jukebox/tracklist.h>


int TrackList::ProcessInput(const char *pending) {

  if (IS_LETTER(pending))
    return JumpToLetter(pending[0]);

  return AStaticList::ProcessInput(pending); 
} /* ProcessInput() */

int TrackList::FormatEntry(char *str, int deck, int disc, int track,
                           char *title, char *artist) {

  sprintf(str, " %c %3d %3d %-66s", deck + 0x41, disc, track, "");
  memcpy(str + 12, title, (strlen(title) <= 34) ? strlen(title) : 34);
  memcpy(str + 48, artist, (strlen(artist) <= 29) ? strlen(artist) : 29);

  return 1;
} /* FormatEntry() */

int TrackList::SetSortBy(int sb) {
  char buf[80];

  sorted_by = sb;
  sprintf(buf, "SetSortBy(): %d %d", sorted_by, sb);
  PostError(buf);
  return 1;
}

int TrackList::JumpToLetter(char letter) {
  char str[80];
  int size;

  letter = tolower(letter);
  AStaticList::line = 0;

  if (sorted_by == 1) {
    ArtistEntry entry;

    ClearList();
    min = cd_library -> QueryLetter(letter, 1);

    if ((max = min + AWindow::h) > (entries - 1)) {
      AStaticList::line = max - entries + 1;
      min = entries - AWindow::h - 1;
      max = min + AWindow::h;
    }

    if (AWindow::h < (size = artists -> ListSize()))
      size = AWindow::h;

    for (int i = 0; i < size; i++) {
      artists -> GetData(i + min, entry, 1);
      FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                  entry.QueryTrack(), entry.QueryTitle(),
                  entry.QueryArtist());
      AddString(str, 0);
    }
  } else {
    if (sorted_by == 2) {
      TitleEntry entry;

      ClearList();
      min = cd_library -> QueryLetter(letter, 0);

      if ((max = min + AWindow::h) > (entries - 1)) {
        AStaticList::line = max - entries + 1;
        min = entries - AWindow::h - 1;
        max = min + AWindow::h;
      }

      if (AWindow::h < (size = titles -> ListSize()))
        size = AWindow::h;

      for (int i = 0; i < size; i++) {
        titles -> GetData(i + min, entry, 1);
        FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                    entry.QueryTrack(), entry.QueryTitle(),
                    entry.QueryArtist());
        AddString(str, 0);
      }
    }
  }

  return Refresh();
} /* JumpToLetter() */

int TrackList::ResetList(void) {
  char str[80], buf[80];
  int size;

  min = 0;
  max = AWindow::h;
  AStaticList::line = 0;
  ClearList();

  sprintf(buf, "ResetList: sortby = %d", sorted_by);
  PostError(buf);

  if (sorted_by == 1) {
    ArtistEntry entry;

    entries = artists -> ListSize();

    if (AWindow::h < (size = artists -> ListSize()))
      size = AWindow::h;

    for (int i = 0; i < size; i++) {
      artists -> GetData(i, entry, 1);
      FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                  entry.QueryTrack(), entry.QueryTitle(),
                  entry.QueryArtist());
      AddString(str, 0);
    }
  } else {
    if (sorted_by == 2) {
      TitleEntry entry;

      entries = titles -> ListSize();

      if (AWindow::h < (size = titles -> ListSize()))
        size = AWindow::h;

      sprintf(buf, "SortBy Title: size = %d", size);
      PostError(buf);

      for (int i = 0; i < size; i++) {
        titles -> GetData(i, entry, 1);
        FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                    entry.QueryTrack(), entry.QueryTitle(),
                    entry.QueryArtist());
        AddString(str, 0);
      }
    } else {
      DeckDiscTrackEntry entry;

      entries = ddt -> ListSize();

      if (AWindow::h < (size = ddt -> ListSize()))
        size = AWindow::h;

      for (int i = 0; i < size; i++) {
        ddt -> GetData(i, entry, 1);
        FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                    entry.QueryTrack(), entry.QueryTitle(),
                    entry.QueryArtist());
        AddString(str, 0);
      }
    }
  }

  return Refresh();
} /* ResetList() */

int TrackList::Up(void) {
  char str[80];

  if (line > 0)
    return AStaticList::Up();

  if (!min)
    return 0;

  RemoveString(AWindow::h - 1);
  min--;
  max--;

  if (sorted_by == 1) {
    ArtistEntry entry;

    artists -> GetData(min, entry, 1);
    FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                  entry.QueryTrack(), entry.QueryTitle(),
                  entry.QueryArtist());
    AddString(str, 2);
  } else {
    if (sorted_by == 2) {
      TitleEntry entry;

      titles -> GetData(min, entry, 1);
      FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                  entry.QueryTrack(), entry.QueryTitle(),
                  entry.QueryArtist());
      AddString(str, 2);
    } else {
      DeckDiscTrackEntry entry;

      ddt -> GetData(min, entry, 1);
      FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                  entry.QueryTrack(), entry.QueryTitle(),
                  entry.QueryArtist());
      AddString(str, 2);
    }
  }

  return Refresh();
} /* Up() */

int TrackList::Down(void) {
  char str[80];
  char text[80];

  if ((entries - 3) == (min + AStaticList::line))
    return 0;

  if (AStaticList::line < (AWindow::h - 1))
    return AStaticList::Down();

  RemoveString(0);
  AStaticList::line = AWindow::h - 1;
  min++;
  max++;

  sprintf(text, "Down - S: %d  L: %d  Min: %d  Max: %d",
          AStaticList::select, AStaticList::line, min, max);
  PostError(text);

  if (sorted_by == 1) {
    ArtistEntry entry;

    artists -> GetData(max, entry, 1);
    FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                entry.QueryTrack(), entry.QueryTitle(),
                entry.QueryArtist());
    AddString(str, 0);
  } else {
    if (sorted_by == 2) {
      TitleEntry entry;

      titles -> GetData(max, entry, 1);
      FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                  entry.QueryTrack(), entry.QueryTitle(),
                  entry.QueryArtist());
      AddString(str, 0);
    } else {
      DeckDiscTrackEntry entry;

      ddt -> GetData(max, entry, 1);
      FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                  entry.QueryTrack(), entry.QueryTitle(),
                  entry.QueryArtist());
      AddString(str, 0);
    }
  }

  return Refresh();
} /* Down() */

int TrackList::PageUp(void) {
  char str[80];
  int size;

  if (!min) {
    AStaticList::line = 0;
    return Refresh();
  }

  if (sorted_by == 1) {
    ArtistEntry entry;

    if ((min -= (AWindow::h - 1)) < 0)
      min = 0;

    max = min + AWindow::h;
    ClearList();

    if (AWindow::h < (size = artists -> ListSize()))
      size = AWindow::h;

    for (int i = 0; i < size; i++) {
      artists -> GetData(i + min, entry, 1);
      FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                  entry.QueryTrack(), entry.QueryTitle(),
                  entry.QueryArtist());
      AddString(str, 0);
    }
  } else {
    if (sorted_by == 2) {
      TitleEntry entry;

      if ((min -= (AWindow::h - 1)) < 0)
        min = 0;

      max = min + AWindow::h;
      ClearList();

      if (AWindow::h < (size = titles -> ListSize()))
        size = AWindow::h;

      for (int i = 0; i < size; i++) {
        titles -> GetData(i + min, entry, 1);
        FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                    entry.QueryTrack(), entry.QueryTitle(),
                    entry.QueryArtist());
        AddString(str, 0);
      }
    } else {
      DeckDiscTrackEntry entry;

      if ((min -= (AWindow::h - 1)) < 0)
        min = 0;

      max = min + AWindow::h;
      ClearList();

      if (AWindow::h < (size = ddt -> ListSize()))
        size = AWindow::h;

      for (int i = 0; i < size; i++) {
        ddt -> GetData(i + min, entry, 1);
        FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                    entry.QueryTrack(), entry.QueryTitle(),
                    entry.QueryArtist());
        AddString(str, 0);
      }
    }
  }

  return Refresh();
} /* PageUp() */

int TrackList::PageDown(void) {
  char str[80];
  int size;

  if ((entries - 1) == max) {
    AStaticList::line = AWindow::h - 1;
    return Refresh();
  }

  if (sorted_by == 1) {
    ArtistEntry entry;

    if ((max += AWindow::h) > (entries - 1))
      min = entries - AWindow::h - 1;
    else
      min = min + (AWindow::h - 1);

    max = min + AWindow::h;
    ClearList();

    if (AWindow::h < (size = artists -> ListSize()))
      size = AWindow::h;

    for (int i = 0; i < size; i++) {
      artists -> GetData(i + min, entry, 1);
      FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                  entry.QueryTrack(), entry.QueryTitle(),
                  entry.QueryArtist());
      AddString(str, 0);
    }
  } else {
    if (sorted_by == 2) {
      TitleEntry entry;

      if ((max += AWindow::h) > (entries - 1))
        min = entries - AWindow::h - 1;
      else
        min = min + (AWindow::h - 1);

      max = min + AWindow::h;
      ClearList();

      if (AWindow::h < (size = titles -> ListSize()))
        size = AWindow::h;

      for (int i = 0; i < size; i++) {
        titles -> GetData(i + min, entry, 1);
        FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                    entry.QueryTrack(), entry.QueryTitle(),
                    entry.QueryArtist());
        AddString(str, 0);
      }
    } else {
      DeckDiscTrackEntry entry;

      if ((max += AWindow::h) > (entries - 1))
        min = entries - AWindow::h - 1;
      else
        min = min + (AWindow::h - 1);

      max = min + AWindow::h;
      ClearList();

      if (AWindow::h < (size = ddt -> ListSize()))
        size = AWindow::h;

      for (int i = 0; i < size; i++) {
        ddt -> GetData(i + min, entry, 1);
        FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                    entry.QueryTrack(), entry.QueryTitle(),
                    entry.QueryArtist());
        AddString(str, 0);
      }
    }
  }

  return Refresh();
} /* PageDown() */

int TrackList::Home(void) {
  char str[80];
  int size;

  min = 0;
  max = AWindow::h;
  AStaticList::line = 0;
  ClearList();

  if (sorted_by == 1) {
    ArtistEntry entry;

    if (AWindow::h < (size = artists -> ListSize()))
      size = AWindow::h;

    for (int i = 0; i < size; i++) {
      artists -> GetData(i, entry, 1);
      FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                  entry.QueryTrack(), entry.QueryTitle(),
                  entry.QueryArtist());
      AddString(str, 0);
    }
  } else {
    if (sorted_by == 2) {
      TitleEntry entry;

      if (AWindow::h < (size = titles -> ListSize()))
        size = AWindow::h;

      for (int i = 0; i < size; i++) {
        titles -> GetData(i, entry, 1);
        FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                    entry.QueryTrack(), entry.QueryTitle(),
                    entry.QueryArtist());
        AddString(str, 0);
      }
    } else {
      DeckDiscTrackEntry entry;

      if (AWindow::h < (size = ddt -> ListSize()))
        size = AWindow::h;

      for (int i = 0; i < size; i++) {
        ddt -> GetData(i, entry, 1);
        FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                    entry.QueryTrack(), entry.QueryTitle(),
                    entry.QueryArtist());
        AddString(str, 0);
      }
    }
  }

  return Refresh();
} /* Home() */

int TrackList::End(void) {
  char str[80];
  int size;

  min = entries - (AWindow::h + 1);
  max = min + AWindow::h;
  AStaticList::line = AWindow::h - 1;
  ClearList();

  if (sorted_by == 1) {
    ArtistEntry entry;

    if (AWindow::h < (size = artists -> ListSize()))
      size = AWindow::h;

    for (int i = 0; i < size; i++) {
      artists -> GetData(min + i, entry, 1);
      FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                  entry.QueryTrack(), entry.QueryTitle(),
                  entry.QueryArtist());
      AddString(str, 0);
    }
  } else {
    if (sorted_by == 2) {
      TitleEntry entry;

      if (AWindow::h < (size = titles -> ListSize()))
        size = AWindow::h;

      for (int i = 0; i < size; i++) {
        titles -> GetData(min + i, entry, 1);
        FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                    entry.QueryTrack(), entry.QueryTitle(),
                    entry.QueryArtist());
        AddString(str, 0);
      }
    } else {
      DeckDiscTrackEntry entry;

      if (AWindow::h < (size = ddt -> ListSize()))
        size = AWindow::h;

      for (int i = 0; i < size; i++) {
        ddt -> GetData(min + i, entry, 1);
        FormatEntry(str, entry.QueryDeck(), entry.QueryDisc(),
                    entry.QueryTrack(), entry.QueryTitle(),
                    entry.QueryArtist());
        AddString(str, 0);
      }
    }
  }

  return Refresh();
} /* End() */

int TrackList::OnSelect(void) {
  int deck = 0, disc = 1, track = 1;
  struct track *trk;
  char field[80], text[80];

  if (sorted_by == 1) {
    ArtistEntry entry;

    if (AStaticList::line == (AWindow::h - 1))
      artists -> GetData(min + SelectedIndex() + 1, entry, 1);
    else
      artists -> GetData(min + SelectedIndex(), entry, 1);

    deck = entry.QueryDeck();
    disc = entry.QueryDisc();
    track = entry.QueryTrack();
    FormatEntry(field, deck, disc, track,
      entry.QueryTitle(), entry.QueryArtist());
  } else {
    if (sorted_by == 2) {
      TitleEntry entry;

      if (AStaticList::line == (AWindow::h - 1))
        titles -> GetData(min + SelectedIndex() + 1, entry, 1);
      else
        titles -> GetData(min + SelectedIndex(), entry, 1);

      deck = entry.QueryDeck();
      disc = entry.QueryDisc();
      track = entry.QueryTrack();
      FormatEntry(field, deck, disc, track,
        entry.QueryTitle(), entry.QueryArtist());
    } else {
      DeckDiscTrackEntry entry;

      if (AStaticList::line == (AWindow::h - 1))
        ddt -> GetData(min + SelectedIndex() + 1, entry, 1);
      else
        ddt -> GetData(min + SelectedIndex(), entry, 1);

      deck = entry.QueryDeck();
      disc = entry.QueryDisc();
      track = entry.QueryTrack();
      FormatEntry(field, deck, disc, track,
        entry.QueryTitle(), entry.QueryArtist());
    }
  }

  sprintf(text, "Select - S: %d  L: %d  Min: %d  Max %d  Deck: %d  "
          "Disc: %d  Track: %d", AStaticList::select, AStaticList::line,
          min, max, deck, disc, track);
  PostError(text);
  PostError(field);

  if ((trk = cd_library -> QueryTrack(deck, disc, track)) == NULL)
    return 0;

  PostError(text);
  PostError(field);

  queues[deck] -> AddEntry(field, trk, 0);
  queues[deck] -> Refresh();

  // Prequeue it, if it's the only track pending and the deck is idle
  if ((queues[deck] -> ListSize() == 1) &&
      ((cd_player -> QueryDeck(deck)) -> state != STATE_PLAYING))
    cd_player -> Pause(deck, disc, track);

  return AStaticList::OnSelect();
} /* OnSelect() */

int TrackList::Refresh(void) {
  char attr[40], color[20], loc[10], format[40], *ptr;
  int i;

  AWindow::Attr(attr);

  sprintf(format, SAVE_LOC "%%s%%s%%s%%s%%-%ds" RESET_ASC FG BG 
          "%%c" LOAD_LOC, AWindow::w - 1, BLACK, WHITE);

  for (i = 0; i < AWindow::h; i++) {
    AWindow::Loc(loc, AWindow::x, AWindow::y + i);
    ptr = ((AStaticList::select + i) < ListSize()) ?
           (Index(AStaticList::select + i)) -> Text() : strdup("  ");
    sprintf(color, RESET_ASC BG FG, BLACK, (ptr[1] == 'A' ? GREEN :
            (ptr[1] == 'B' ? RED : BLACK)));

    if (ActiveWindow() == WindowID())
      fprintf(stdout, format, attr, color, loc, (line == i) ?
              REVERSE_ASC : "", ptr, !i ? 45 : (( ((i == 1) &&
              (AStaticList::select > 0)) || ((i == (AWindow::h - 1)) &&
              (AStaticList::select + AWindow::h < ListSize()))) ? 254 : 32));
    else
      fprintf(stdout, format, attr, color, loc, "", ptr, !i ? 45 :
              (( ((i == 1) && (AStaticList::select > 0)) || 
              ((i == (AWindow::h - 1)) && (AStaticList::select +
              AWindow::h < ListSize()))) ? 254 : 32));
  }

  AWindow::Loc(loc, AWindow::x + AWindow::w - 1, AWindow::y);
  fprintf(stdout, "%s", loc);

  return 1;
} /* Refresh() */

#endif // TRACK_LIST_BOX

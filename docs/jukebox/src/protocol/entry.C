#ifndef CDDB_ENTRY_CLASS
#define CDDB_ENTRY_CLASS

#include <protocol/entry.h>

CDDBEntry &CDDBEntry::operator= (const CDDBEntry& p) {

  if (&p != this) {
    cddb = p.cddb;
    offsets = p.offsets;
  }

  return *this;
} /* operator=() */

int CDDBEntry::operator== (const CDDBEntry& p) const {
  return (cddb == p.cddb);
} /* operator==() */

int CDDBEntry::operator!= (const CDDBEntry& p) const {
  return (cddb != p.cddb);
} /* operator!=() */

int CDDBEntry::operator< (const CDDBEntry& p) const {
  return (cddb < p.cddb);
} /* operator<() */

int CDDBEntry::operator<= (const CDDBEntry& p) const {
  return (cddb <= p.cddb);
} /* operator<=() */

int CDDBEntry::operator> (const CDDBEntry& p) const {
  return (cddb > p.cddb);
} /* operator>() */

int CDDBEntry::operator>= (const CDDBEntry& p) const {
  return (cddb >= p.cddb);
} /* operator>=() */

#endif /* CDDB_ENTRY_CLASS */

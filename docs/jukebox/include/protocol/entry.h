#ifndef CDDB_ENTRY_CLASS_H
#define CDDB_ENTRY_CLASS_H

#include "ds/linkedlist.h"

class CDDBEntry {
  private:
    long cddb;
    LinkedList<long> offsets;

  public:
    CDDBEntry(void) { cddb = 0; }
    CDDBEntry(long id, long off) { cddb = id; offsets.InsertRear(off); }
    ~CDDBEntry(void) { }

    CDDBEntry &operator= (const CDDBEntry&);
    int operator== (const CDDBEntry&) const;
    int operator!= (const CDDBEntry&) const;
    int operator< (const CDDBEntry&) const;
    int operator<= (const CDDBEntry&) const;
    int operator> (const CDDBEntry&) const;
    int operator>= (const CDDBEntry&) const;

    void SetCDDB(long id) { cddb = id; }
    void AddOffset(long off) { offsets.InsertRear(off); }

    long QueryCDDB(void) { return cddb; }
    LinkedList<long> *QueryOffsets(void) { return &offsets; }
    long QueryOffset(int index) { return (long)offsets.Index(index); }
    int OffsetSize(void) { return offsets.ListSize(); }

}; /* class CDDBEntry */

#endif /* CDDB_ENTRY_CLASS_H */

#ifndef LINKEDLIST_CLASS_H
#define LINKEDLIST_CLASS_H

// #include <iostream.h>
#include <stdlib.h>
#include "ds/listnode.h"

template <class T>
class SeqListIterator;

template <class T>
class LinkedList {
  private:
    Node<T> *front, *rear;
    Node<T> *prevPtr, *currPtr;
    int size, position;

    Node<T> *GetNode(const T&, Node<T> *ptrNext = NULL);
    void FreeNode(Node<T> *);
    void CopyList(const LinkedList<T>&);
      
  public:
    LinkedList(void);
    LinkedList(const LinkedList<T>&);
    ~LinkedList(void);
      
    LinkedList<T>& operator= (const LinkedList<T>& L);
    T& operator[](int);
    T& Index(int);
      
    int ListSize(void) const;               
    int ListEmpty(void) const;

    void ClearList(void);      
    int Reset(int pos = 0);
    void Next(void);
    int EndOfList(void) const;
    int CurrentPosition(void) const;
      
    T& Data(void);

    void InsertFront(const T&);
    void InsertRear(const T&);
    void InsertAt(const T&);
    void InsertAfter(const T&);
    T DeleteFront(void);
    T DeleteAt(void);

    friend class SeqListIterator<T>;
}; /* class LinkedList */

#endif  // LINKEDLIST_CLASS_H

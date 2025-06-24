#ifndef LINKEDLIST_CLASS
#define LINKEDLIST_CLASS

// #include <iostream.h>
#include <stdlib.h>
#include "ds/linkedlist.h"
#include "ds/listnode.h"
#include <jukebox.h>

template <class T>
class SeqListIterator;


template <class T>
Node<T> *LinkedList<T>::GetNode(const T& item, Node<T>* ptrNext) {
  Node<T> *p = new Node<T>(item,ptrNext);

  if (p == NULL) 
    return NULL;

  return p;
} /* GetNode() */

template <class T>
void LinkedList<T>::FreeNode(Node<T> *p) { delete p; }

template <class T>
void LinkedList<T>::CopyList(const LinkedList<T>& L) {
  Node<T> *p = L.front;
  int pos;

  while (p != NULL) {
    InsertRear(p -> data);
    p = p -> NextNode();
  }
   
  if (position == -1)
    return;

  prevPtr = NULL;
  currPtr = front;

  for (pos = 0; pos != position; pos++) {
    prevPtr = currPtr;
    currPtr = currPtr -> NextNode();
  }
} /* CopyList() */

// Constructor
template <class T>
LinkedList<T>::LinkedList(void): front(NULL), rear(NULL),
    prevPtr(NULL),currPtr(NULL), size(0), position(-1) { }

// Constructor
template <class T>
LinkedList<T>::LinkedList(const LinkedList<T>& L) {
  front = rear = NULL;
  prevPtr = currPtr = NULL;
  size = 0;
  position = -1;
  CopyList(L);
} /* LinkedLink() */

template <class T>
LinkedList<T>::~LinkedList(void) { ClearList(); }

template <class T>
void LinkedList<T>::ClearList(void) {
  Node<T> *currPosition = front, *nextPosition;

  while(currPosition != NULL) {
    nextPosition = currPosition -> NextNode();
    FreeNode(currPosition);
    currPosition = nextPosition;
  }

  front = rear = NULL;
  prevPtr = currPtr = NULL;
  size = 0;
  position = -1;
} /* ClearList() */

template <class T>
LinkedList<T>& LinkedList<T>::operator= 
               (const LinkedList<T>& L) {

  if (this == &L)
    return *this;

  ClearList();
  CopyList(L);
  return *this;
} /* operator= */

template <class T>
T& LinkedList<T>::operator[](int index) {
  Reset(index);
  return Data();
} /* operator[] */

template <class T>
T& LinkedList<T>::Index(int index) {
  Reset(index);
  return Data();
} /* Index() */

template <class T>
int LinkedList<T>::ListSize(void) const { return size; }
                      
template <class T>
int LinkedList<T>::ListEmpty(void) const { return size == 0; }

template <class T>
void LinkedList<T>::Next(void) {

  if (currPtr != NULL) {
    prevPtr = currPtr;
    currPtr = currPtr -> NextNode();
    position++;
  }
} /* Next() */

template <class T>
int LinkedList<T>::EndOfList(void) const { return currPtr == NULL; }

template <class T>
int LinkedList<T>::CurrentPosition(void) const { return position; }

template <class T>
int LinkedList<T>::Reset(int pos) {
  int startPos;

  if (front == NULL)
    return -1;
      
  if ((pos < 0) || (pos >= size))
    return -1;

  if (pos == 0) {
    prevPtr = NULL;
    currPtr = front;
    position = 0;
  } else {
    currPtr = front -> NextNode();
    prevPtr = front;
    startPos = 1;

    for (position = startPos; position != pos; position++) {
      prevPtr = currPtr;
      currPtr = currPtr -> NextNode();
    }
  }

  return position;
} /* Reset() */

template <class T>
T& LinkedList<T>::Data(void) {
  static T ret = (T)NULL;

  if (size == 0 || currPtr == NULL)
    return ret;

  return currPtr -> data;
} /* Data() */

template <class T>
void LinkedList<T>::InsertFront(const T& item) {

  if (front != NULL)
    Reset();

  InsertAt(item);
} /* InsertFront() */

template <class T>
void LinkedList<T>::InsertRear(const T& item) {
  Node<T> *newNode;
   
  prevPtr = rear;
  newNode = GetNode(item);

  if (rear == NULL) {
    front = rear = newNode;
  } else {
    rear -> InsertAfter(newNode);
    rear = newNode;
  }

  currPtr = rear;
  position = size;
  size++;
} /* InsertRear() */

template <class T>
void LinkedList<T>::InsertAt(const T& item) {
  Node<T> *newNode;

  if (prevPtr == NULL) {
    newNode = GetNode(item,front);
    front = newNode;
  } else {
    newNode = GetNode(item);
    prevPtr -> InsertAfter(newNode);
  }
   
  if (prevPtr == rear) {
    rear = newNode;
    position = size;
  }

  currPtr = newNode;
  size++;           
} /* InsertAt() */

template <class T>
void LinkedList<T>::InsertAfter(const T& item) {
  Node<T> *p = GetNode(item);

  if (front == NULL) {
    front = currPtr = rear = p;
    position = 0;
  } else {
    if (currPtr == NULL)
      currPtr = prevPtr;
      currPtr -> InsertAfter(p);
    
    if (currPtr == rear) {
      rear = p;
      position = size;
    } else {
      position++;
    }
    
    prevPtr = currPtr;
    currPtr = p;
  }

  size++;
} /* InsertAfter() */

template <class T>
T LinkedList<T>::DeleteFront(void) {
  T item;

  Reset();

  if (front == NULL)
    return (T)NULL;
  
  item = currPtr -> data;
  DeleteAt();

  return item;
} /* DeleteFront() */

template <class T>
T LinkedList<T>::DeleteAt(void) {
  Node<T> *p;
  T item;

  if (currPtr == NULL) 
    return (T)NULL;
   
  if (prevPtr == NULL) {
    p = front;
    front = front -> NextNode();
  } else {
    p = prevPtr -> DeleteAfter();
  }

  if (p == rear) {
    rear = prevPtr;
    Reset(--position);
  }

  currPtr = p -> NextNode();
  item = p -> data;
  FreeNode(p);
  size--;

  return item;
} /* DeleteAt() */

template class LinkedList<AWindow *>;
template class LinkedList<String *>;
template class LinkedList<struct command *>;
template class LinkedList<struct packet *>;
template class LinkedList<struct message *>;
template class LinkedList<struct track *>;
template class LinkedList<struct disc *>;
template class LinkedList<long>;

#endif  // LINKEDLIST_CLASS

#ifndef NODE_CLASS
#define NODE_CLASS

#include "ds/listnode.h"
#include <jukebox.h>

// Constructor
template <class T>
Node<T>::Node(const T& item, Node<T>* ptrnext) {
  data = item;
  next = ptrnext;
} /* Node() */
  
template <class T>
Node<T> *Node<T>::NextNode(void) const { return next; } 

template <class T>
void Node<T>::InsertAfter(Node<T> *p) {
  p -> next = next;
  next = p;
} /* InsertAfter() */

template <class T>
Node<T> *Node<T>::DeleteAfter(void) {
  Node<T> *tempPtr = next;

  if (next == NULL)
    return NULL;
        
  next = tempPtr -> next;
  return tempPtr;
} /* DeleteAfter() */

template class Node<AWindow *>;
template class Node<String *>;
template class Node<struct command *>;
template class Node<struct packet *>;
template class Node<struct message *>;
template class Node<struct track *>;
template class Node<struct disc *>;
template class Node<long>;

#endif  // NODE_CLASS

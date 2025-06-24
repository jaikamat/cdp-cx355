#ifndef BINARY_SEARCH_TREE_CLASS
#define BINARY_SEARCH_TREE_CLASS

#include <stdlib.h>
#include "ds/bstree.h"
#include "ds/treenode.h"
#include <jukebox.h>

// Constructor
template <class T>
BSTree<T>::BSTree(void) : root(NULL), current(NULL), size(0) { }

// Constructor
template <class T>
BSTree<T>::BSTree(const BSTree<T>& tree) {
  root = CopyTree(tree.root);
  current = root;
  size = tree.size;
} /* BSTree() */

// Destructor
template <class T>
BSTree<T>::~BSTree(void) { ClearList(); }

template <class T>
BSTree<T>& BSTree<T>::operator= (const BSTree<T>& rhs) {

  if (this == &rhs)
    return *this;
        
  ClearList();
  root = CopyTree(rhs.root);
    
  current = root;
  size = rhs.size;
    
  return *this;
} /* operator= */

template <class T>
TreeNode<T> *BSTree<T>::GetRoot(void) const {
  return root;
} /* GetRoot() */

template <class T>
TreeNode<T> *BSTree<T>::GetCurrent(void) const {
  return current;
} /* GetRoot() */

template <class T>
void BSTree<T>::ClearList(void) {
  DeleteTree(root);
  root = NULL;
  current = NULL;
  size = 0;
} /* ClearList() */

template <class T>
int BSTree<T>::ListEmpty(void) const {
  return root == NULL;
} /* ListEmpty() */

template <class T>
int BSTree<T>::ListSize(void) const { return size; }

template <class T>
int BSTree<T>::GetData(int node,T &item,int order,TreeNode<T> *ptr,int flag) {
  static int count = -1;

  if (flag) {
    ptr = root;
    count = -1;

    if ((node >= size) || (node < 0))
      return 0;
  }

  switch (order) {
    default:  return 0;

    case 1:  // InOrder Traversel

      if (ptr != NULL) {
        if (GetData(node, item, order, ptr -> left, 0))
          return 1;

        if (node == (count++)) {
          item = ptr -> data;
          return 1;
        }

        if (GetData(node, item, order, ptr -> right, 0))
          return 1;
      }

      break;

    case 2:  // PreOrder Traversel

      if (ptr != NULL) {

        if (node == (count++)) {
          item = ptr -> data;
          return 1;
        }

        if (GetData(node, item, order, ptr -> left, 0))
          return 1;

        if (GetData(node, item, order, ptr -> right, 0))
          return 1;
      }

      break;

    case 3:  // PostOrder Traversel.

      if (ptr != NULL) {
        if (GetData(node, item, order, ptr -> left, 0))
          return 1;

        if (GetData(node, item, order, ptr -> right, 0))
          return 1;

        if (node == (count++)) {
          item = ptr -> data;
          return 1;
        }
      }

      break;
  }

  return 0;
} /* GetData() */

template <class T>
int BSTree<T>::GetCurrentData(T &item) {

  item = current -> data;

  return 1;
} /* GetCurrentData() */

template <class T> 
TreeNode<T> *BSTree<T>::GetTreeNode(const T& item, TreeNode<T> *lptr,
    TreeNode<T> *rptr) {
  TreeNode<T> *p = new TreeNode<T>(item, lptr, rptr);
    
  if (p == NULL) {
    fprintf(stderr, "Memory allocation failure!\n");
    exit(1);
  }
  
  return p;
} /* GetTreeNode() */

template <class T>
void BSTree<T>::FreeTreeNode(TreeNode<T> *p) {
  delete p;
} /* FreeTreeNode() */

template <class T>
TreeNode<T> *BSTree<T>::CopyTree(TreeNode<T> *t) {
  TreeNode<T> *newlptr, *newrptr, *newNode;
   
  if (t == NULL)
    return NULL;
        
  if (t -> left != NULL) 
    newlptr = CopyTree(t -> left);
  else
    newlptr = NULL;
 
  if (t -> right != NULL) 
    newrptr = CopyTree(t -> right);
  else
    newrptr = NULL;
 
  newNode = GetTreeNode(t -> data, newlptr, newrptr);
  return newNode;
} /* CopyTree() */

template <class T>
void BSTree<T>::DeleteTree(TreeNode<T> *t) {

  if (t != NULL) {
    DeleteTree(t -> left);
    DeleteTree(t -> right);
    FreeTreeNode(t);
  }
} /* DeleteTree() */

template <class T>
TreeNode<T> *BSTree<T>::FindNode(const T& item,
    TreeNode<T>* & parent) const {   
  TreeNode<T> *t = root;
    
  parent = NULL;
    
  while(t != NULL)
    if (item == t -> data) {
      break;
    } else {
      parent = t;
  
      if (item < t -> data)
        t = t -> left;
      else 
        t = t -> right;
    }

  return t;
} /* FindNode() */

template <class T>
int BSTree<T>::Find(T& item) {
  TreeNode<T> *parent;

  current = FindNode(item, parent);
    
  if (current != NULL) {
    item = current -> data;
    return 1;
  }

  return 0;
} /* Find() */

template <class T>
void BSTree<T>::Insert(const T& item) {
  TreeNode<T> *t = root, *parent = NULL, *newNode;

  while(t != NULL) {
    parent = t;
  
    if (item < t -> data)
      t = t -> left;
    else 
      t = t -> right;
  }
    
  newNode = GetTreeNode(item, NULL, NULL);
    
  if (parent == NULL)
    root = newNode;
  else
    if (item < parent -> data)                   
      parent -> left = newNode;
    else
      parent -> right = newNode;
        
  current = newNode;
  size++;
} /* Insert() */

template <class T>
void BSTree<T>::Delete(const T& item) {
  TreeNode<T> *DNodePtr, *PNodePtr, *RNodePtr;
    
  if ((DNodePtr = FindNode (item, PNodePtr)) == NULL)
    return;

  if (DNodePtr -> right == NULL) {
    RNodePtr = DNodePtr -> left;
  } else { 
    if (DNodePtr -> left == NULL) {
      RNodePtr = DNodePtr -> right;
    } else {
      TreeNode<T> *PofRNodePtr = DNodePtr;

      RNodePtr = DNodePtr -> left;
    
      while(RNodePtr -> right != NULL) {
        PofRNodePtr = RNodePtr;
        RNodePtr = RNodePtr -> right;
      }
        
      if (PofRNodePtr == DNodePtr) {
        RNodePtr -> right = DNodePtr -> right;
      } else {
        PofRNodePtr -> right = RNodePtr -> left;
        RNodePtr -> left = DNodePtr -> left;
        RNodePtr -> right = DNodePtr -> right;
      }
    }
  }

  if (PNodePtr == NULL)
    root = RNodePtr;
  else 
    if (DNodePtr -> data < PNodePtr -> data)
      PNodePtr -> left = RNodePtr;
    else
      PNodePtr -> right = RNodePtr;

  FreeTreeNode(DNodePtr);
  size--;
} /* Delete() */

template <class T>
void BSTree<T>::Update(const T& item) {   
  
  if (current != NULL && current -> data == item)
    current -> data = item;
  else
    Insert(item);
} /* Update() */

template class BSTree<DeckDiscTrackEntry>;
template class BSTree<TitleEntry>;
template class BSTree<ArtistEntry>;
template class BSTree<CDDBEntry>;


#endif  // BINARY_SEARCH_TREE_CLASS

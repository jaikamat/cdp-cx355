#ifndef TREENODE_CLASS
#define TREENODE_CLASS

template <class T>
class BSTree;

#include "ds/treenode.h"
#include <jukebox.h>

template <class T>
TreeNode<T>::TreeNode (const T& item, TreeNode<T> *lptr, TreeNode<T> *rptr):
  data(item) { left = lptr; right = rptr; }

template <class T>
TreeNode<T>::~TreeNode(void) { }

template <class T>
TreeNode<T>* TreeNode<T>::Left(void) const { return left; }

template <class T>
TreeNode<T>* TreeNode<T>::Right(void) const { return right; }

template class TreeNode<DeckDiscTrackEntry>;
template class TreeNode<TitleEntry>;
template class TreeNode<ArtistEntry>;
template class TreeNode<CDDBEntry>;

#endif // TREENODE_CLASS

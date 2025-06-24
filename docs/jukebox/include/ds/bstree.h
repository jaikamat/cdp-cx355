#ifndef BINARY_SEARCH_TREE_CLASS_H
#define BINARY_SEARCH_TREE_CLASS_H

#include <stdlib.h>
#include "treenode.h"

template <class T>
class BSTree {
  protected:
    TreeNode<T> *root;
    TreeNode<T> *current;
    int size;
    
    TreeNode<T> *CopyTree(TreeNode<T> *);
    TreeNode<T> *FindNode(const T&, TreeNode<T>* &) const;
    TreeNode<T> *GetTreeNode(const T&, TreeNode<T> *, TreeNode<T> *);
    void FreeTreeNode(TreeNode<T> *);
    void DeleteTree(TreeNode<T> *);

  public:
    BSTree(void);
    BSTree(const BSTree<T>&);
    ~BSTree(void);
        
    BSTree<T>& operator= (const BSTree<T>&);

    TreeNode<T> *GetRoot(void) const;
    TreeNode<T> *GetCurrent(void) const;
    void ClearList(void);
    int ListEmpty(void) const;
    int ListSize(void) const;
    int GetData(int, T &, int, TreeNode<T> * = NULL, int = 1);
    int GetCurrentData(T &);
    int Find(T&);
    void Insert(const T&);
    void Delete(const T&);
        
    void Update(const T& item);
}; /* class BSTree */

#endif  // BINARY_SEARCH_TREE_CLASS_H

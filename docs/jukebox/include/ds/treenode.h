#ifndef TREENODE_CLASS_H
#define TREENODE_CLASS_H

template <class T>
class BSTree;

template <class T>
class TreeNode {
  protected:
    TreeNode<T> *left;
    TreeNode<T> *right;

  public:
    T data;

    TreeNode(const T&, TreeNode<T> *lptr=NULL, TreeNode<T> *rptr=NULL);
    virtual ~TreeNode(void);

    TreeNode<T>* Left(void) const;
    TreeNode<T>* Right(void) const;
   
  friend class BSTree<T>;
}; /* class TreeNode() */

#endif // TREENODE_CLASS_H

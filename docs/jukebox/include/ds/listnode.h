#ifndef NODE_CLASS_H
#define NODE_CLASS_H

template <class T>
class Node {
  private:
    Node<T> *next;
  public:
    T data;
    
    Node (const T&, Node<T>* ptrnext = NULL);

    Node<T> *NextNode(void) const;

    void InsertAfter(Node<T> *p);
    Node<T> *DeleteAfter(void);        
}; /* class Node */

#endif  // NODE_CLASS_H

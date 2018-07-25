/// \file TreeWrap.hh Utility wrapper to organize node classes into tree structure
// Michael P. Mendenhall, LLNL 2018

#ifndef TREEWRAP_HH
#define TREEWRAP_HH

#include <boost/core/noncopyable.hpp>
#include <vector>
using std::vector;

/// Tree wrapper class
template<class T>
class TreeWrap: public T, private boost::noncopyable {
public:
    /// Destructor, manages child nodes
    virtual ~TreeWrap() { for(auto c: children) delete c; }

    /// top-down iteration over tree nodes
    template<typename ptr_t>
    class top_iterator_t {
    public:
        /// constructor starting at node
        top_iterator_t(ptr_t n = nullptr): N(n) { if(n) position.push_back(0); }
        /// dereference to get node
        ptr_t operator*() { return N; }
        /// crawl to next node
        top_iterator_t& operator++() {
            if(!N || !position.size()) return *this;
            if(position.back() < (int)N->children.size()) {
                N = N->children[position.back()++];
                position.push_back(0);
            } else {
                position.pop_back();
                N = N->parent;
            }
            return *this;
        }
        /// check if iterators unequal
        bool operator!=(const top_iterator_t& rhs) const { return N != rhs.N || position != rhs.position; }
    protected:
        ptr_t N;                    ///< current node
        vector<int> position;       ///< history of branches taken
    };

    /// top-down iterator
    typedef top_iterator_t<TreeWrap<T>*> iterator;
    /// top-down const iterator
    typedef top_iterator_t<const TreeWrap<T>*> const_iterator;

    /// tree iteration beginning
    iterator begin() { return iterator(this); }
    /// tree iteration end
    iterator end()   { return iterator(nullptr); }
    /// tree iteration beginning
    const_iterator cbegin() const { return const_iterator(this); }
    /// tree iteration end
    const_iterator cend() const { return const_iterator(nullptr); }

    /// get parent
    TreeWrap<T>* getParent() { return parent; }
    /// convenience for adding children
    template<class U>
    U* addChild(U* W) { if(W) { children.push_back(W); W->parent = this; } return W; }

protected:
    TreeWrap<T>* parent = nullptr;  ///< parent node
    vector<TreeWrap<T>*> children;  ///< child nodes
};

#endif
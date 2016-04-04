#ifndef _std_h_
#define _std_h_
#include <map>
#include <memory>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using std::const_pointer_cast;
using std::map;
using std::set;
using std::unordered_map;
using std::unordered_set;
//using std::vector;
//using std::array;

template<class T, class _Alloc = std::allocator<T>>
class vector : public std::vector<T, _Alloc> {
public:
    using std::vector<T, _Alloc>::vector;
    typedef typename std::vector<T, _Alloc>::reference reference;
    typedef typename std::vector<T, _Alloc>::const_reference const_reference;
    typedef typename std::vector<T, _Alloc>::size_type size_type;
    typedef typename std::vector<T>::const_iterator const_iterator;
    reference operator[](size_type n) { return this->at(n); }
    const_reference operator[](size_type n) const { return this->at(n); }
};

template<class T, size_t N>
class array : public std::array<T, N> {
public:
    using std::array<T, N>::array;
    typedef typename std::array<T, N>::reference reference;
    typedef typename std::array<T, N>::const_reference const_reference;
    typedef typename std::array<T, N>::size_type size_type;
    reference operator[](size_type n) { return this->at(n); }
    const_reference operator[](size_type n) const { return this->at(n); }
};

#endif /* _std_h_ */

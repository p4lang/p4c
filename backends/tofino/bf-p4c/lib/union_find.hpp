/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef EXTENSIONS_BF_P4C_LIB_UNION_FIND_HPP_
#define EXTENSIONS_BF_P4C_LIB_UNION_FIND_HPP_

#include <initializer_list>
#include <boost/iterator/indirect_iterator.hpp>

#include "bf-p4c/lib/assoc.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/ordered_map.h"
#include "lib/ordered_set.h"

using namespace P4;
/** Implements the Union-Find data structure and algorithm.
 *
 * Construction: O(n), where n is the number of elements.
 * Union(a, b):  O(m * log(n)), where m is the size of the smaller set, n the
 *               larger.
 * Find(x):      O(log(n))
 *
 * TODO: there are more sophisticated implementations that are more
 *            efficient.
 *
 * @param T the type of elements to form into sets.  Must be eligible for
 *          placement in Set-typed containers.
 * @param T optional type of sets to form.  Must be able to hold elements of
 *          type T.
 */
template <typename T, typename Set = ordered_set<T>>
class UnionFind {
    // Map elements to the set that contains them.  Each element is in exactly
    // one set, and set pointers are never null.
    assoc::hash_map<T, Set*> element_map_i;

    // All the sets.
    ordered_set<Set*> sets_i;

    // Used internally for finding and mutating sets; clients cannot mutate
    // sets.
    Set* internalFind(const T x) const {
        auto it = element_map_i.find(x);
        BUG_CHECK(it != element_map_i.end(),
                  "UnionFind: looking for element not in data structure: %1%\n"
                  "UnionFind: data structure is\n%2%",
                  cstring::to_cstring(x), cstring::to_cstring(this));
        return it->second;
    }

 public:
    /// Creates an empty UnionFind.  Elements can be added with UnionFind::insert.
    UnionFind() { }
    /// Copy union find. The resulting struture is independent of the original, it does not share
    /// the sets of elements.
    UnionFind(const UnionFind& other) {
        // The order is important here as it gives iteration order of the UnionFind. We keep the
        // order from the original instance to keep the compiler deterministic.
        for (auto* src : other.sets_i) {
            Set* tgt = new Set(*src);
            sets_i.insert(tgt);
            for (auto& val : *src) {
                element_map_i[val] = tgt;
            }
        }
    }
    UnionFind(UnionFind&&) = default;

    UnionFind& operator=(UnionFind&&) = default;
    UnionFind& operator=(const UnionFind& other) {
        UnionFind copy(other);
        *this = std::move(copy);
        return *this;
    }

    /// we expose iterator to constant sets of elements that are equivalent
    /// Note that the value type given to indirect_iterator must be const so that reference and
    /// pointer typedefs are const-qualified (pointer cannot be specified explicitly).
    using const_iterator = boost::indirect_iterator<typename ordered_set<Set*>::const_iterator,
                                                    const Set>;
    using iterator = const_iterator;
    /// Iterates through the sets in this data structure.
    const_iterator begin() const { return const_iterator(sets_i.begin()); }
    const_iterator end()   const { return const_iterator(sets_i.end()); }

    /// Creates a UnionFind initialized with \p elements.
    template <typename ContainerOfT>
    explicit UnionFind(ContainerOfT elements) {
        for (auto& elt : elements)
            insert(elt);
    }

    /// Creates a UnionFind initialized with \p elements.
    /// Has to be overloaded extra to allow construction like UnionFind uf{1, 2, 3} to work
    explicit UnionFind(std::initializer_list<T> elements) {
        for (auto& elt : elements)
            insert(elt);
    }

    /// Clears the union find data structure
    void clear() {
        element_map_i.clear();
        sets_i.clear();
    }

    /// Adds a new element.  Has no effect if \p elt is already present.
    void insert(const T& elt) {
        if (element_map_i.find(elt) != element_map_i.end())
            return;

        auto* set = new Set();
        set->insert(elt);
        sets_i.insert(set);
        element_map_i[elt] = set;
    }

    /// Unions the sets containing \p x and \p y.
    /// Fails (triggers BUG_CHECK) if \p x or \p y are not present.
    void makeUnion(const T& x, const T& y) {
        Set* xs = internalFind(x);
        Set* ys = internalFind(y);

        if (xs == ys)
            return;

        Set* smaller = xs->size() < ys->size() ? xs : ys;
        Set* larger = xs->size() >= ys->size() ? xs : ys;

        // Union the smaller set with the larger and update the element map.
        for (auto elt : *smaller) {
            larger->insert(elt);
            element_map_i[elt] = larger; }

        // Remove the smaller set.
        sets_i.erase(sets_i.find(smaller));
    }

    /// Unions the sets containing \p x and \p y.
    /// Inserts the elements if they are not present
    void insertUnion(const T& x, const T& y) {
        insert(x);
        insert(y);
        makeUnion(x, y);
    }

    /// @returns a canonical element of the set containing @p x.
    /// Fails (triggers BUG_CHECK) if @p x is not present.
    const T find(const T& x) const {
        Set* internal = internalFind(x);
        return *internal->begin();
    }

    /// @returns the size of the set containing @p x.
    /// Fails (triggers BUG_CHECK) if @p x is not present.
    size_t sizeOf(const T& x) const {
        return internalFind(x)->size();
    }

    /// @returns a copy of the set containing @p x.
    /// Fails (triggers BUG_CHECK) if @p x is not present.
    const Set& setOf(const T& x) const {
        return *internalFind(x);
    }

    /// Indexing behaves similarly to \ref setOf, but when the element \p x is not present, it is
    /// inserted (as a singleton element) -- therefore it works similarly as std::map::operator[].
    const Set& operator[](const T&x) {
        insert(x);
        return setOf(x);
    }

    /// @returns true if element@e is present in the UnionFind struct.
    /// @returns false otherwise.
    bool contains(const T& x) const {
        return element_map_i.count(x);
    }

    /// @returns the size of the largest set in the UnionFind struct.
    size_t maxSize() const {
        size_t rv = 0;
        for (const auto* s : sets_i)
            if (s->size() > rv)
                rv = s->size();
        return rv;
    }

    /// The number of distinct sets of equivalent elements. This can be 0 if and only if the
    /// UnionFind is empty.
    size_t numSets() const {
        return sets_i.size();
    }

    /// The number of elements in the UnionFind.
    size_t size() const {
        return element_map_i.size();
    }
};


template<typename T>
std::ostream &operator<<(std::ostream &out, const UnionFind<T>& uf) {
    for (auto& set : uf) {
        for (auto& x : set)
            out << x << " ";
        out << std::endl;
    }
    return out;
}

template<typename T>
std::ostream &operator<<(std::ostream &out, const UnionFind<T>* uf) {
    if (uf)
        out << *uf;
    else
        out << "-null-union-find-";
    return out;
}


#endif  /* EXTENSIONS_BF_P4C_LIB_UNION_FIND_HPP_ */

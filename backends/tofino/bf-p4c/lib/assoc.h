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

#pragma once

#include <lib/cstring.h>
#include <lib/log.h>
#include <lib/map.h>
#include <lib/ordered_map.h>
#include <lib/ordered_set.h>

#include <map>
#include <set>
#include <string>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>

#include <boost/functional/hash.hpp>

/// The assoc namespace contains wrappers over various associative containers that can be used more
/// safely in the compiler. In particular, it tries to keep track of possible sources of
/// nondeterminism -- cases when repeated compilation of the same P4 code produces different
/// results. If using the containers from assoc namespace, the code does not compile if it iterates
/// over sorted containers that use pointers as keys or if it iterates over unordered containers.
/// The reason is that such iteration often introduces dependency on particular ordering of
/// pointers that can differ between runs of the compiler.
///
/// The downside of this approach is that it is an approximation -- there can be cases when this is
/// perfectly correct and does not introduce nondeterminism: e.g. if the code looks for if some
/// value in a map matches some predicate (but does not return the actual value).
///
/// In general, the iteration (using container.begin()) is forbidden for:
/// - all unsorted (hash-based) containers;
/// - containers that have pointers or tuples/pairs containing pointers as keys, if the containers
///   do not use explicit non-standard comparator.
///
/// This behaviour can be overridden by:
/// - using .unstable_iterable() on the container at the place where it is actually safe to iterate
///   as the result does not really depend on the iteration order.
/// - using .unstable_begin() (together with .end()) in similar cases e.g. in standard algorithms
///   such as std::any_of/std::all_of/std::none_of.
/// - declaring the data structure as assoc::iterable_* (e.g. assoc::iterable_map) to allow the
///   iteration always (not recommended).
namespace assoc {

/// Used to specify if the associative container should be iterable
///
/// - Yes: always iterable
/// - No: always not iterable
/// - Auto: iterable for sorted containers over keys that are not pointers or tuples/pairs
///   containing pointers.
enum class Iterable { Auto, No, Yes };

#define BFN_ASSOC_OP_DEF(name, OP)                                                 \
    friend bool operator OP(const name &lhs, const name &rhs) {                    \
        return static_cast<const ABase &>(lhs) OP static_cast<const ABase &>(rhs); \
    }

/// Implementation details of for the namespace assoc
namespace detail {

template <typename T>
struct _is_stable : std::true_type {};

/// A trait to distinguish types with (hopefully) stable ordering from types that do not have stable
/// ordering. We expect that pointers (and tuples thereof) do not have stable ordering.
template <typename T>
struct is_stable : _is_stable<typename std::decay<T>::type> {};

template <typename T>
struct _is_stable<T *> : std::false_type {};

#if 0
/// To achieve stability even under renaming it would be useful to also consider string not stable
/// in some places (i.e. when strings come from the program). But so far this is too coarse
/// approximation.
template<>
struct _is_stable<std::string> : std::false_type { };

template<>
struct _is_stable<cstring> : std::false_type { };

// TODO(C++update): string_view
#endif

template <typename T1, typename T2>
struct _is_stable<std::pair<T1, T2>>
    : std::integral_constant<bool, _is_stable<T1>::value && is_stable<T2>::value> {};

template <>
struct _is_stable<std::tuple<>> : std::true_type {};

template <typename TH, typename... Ts>
struct _is_stable<std::tuple<TH, Ts...>>
    : std::integral_constant<bool, is_stable<TH>::value && _is_stable<std::tuple<Ts...>>::value> {};

template <typename... Ts>
struct _void {
    using type = void;
};

/// Base of our conditionally-iterable associative containers used for unsorted (hashed)
/// containers. In this case, the iteration order is never stable, even for containers over stable
/// elements. Therefore, we just disable iteration, except if explicitly enabled.
template <typename Base, Iterable Itble, typename = void>
class CondIterableAssocBase : protected Base {
 public:
    using hasher = typename Base::hasher;
    using key_equal = typename Base::key_equal;

    using Base::Base;

 public:
    /// Hash based containers are never iterable unless explicitly enabled. The ordering is never
    /// stable and the order of element in the container might even change if some other elements
    /// are inserted and/or deleted.
    static constexpr bool iterable = Itble == Iterable::Yes;
};

/// Base of our conditionally-iterable associative containers used for sorted containers. For
/// elements with stable sorting this can be iterated over unless explicitly prohibited.
template <typename Base, Iterable Itble>
class CondIterableAssocBase<Base, Itble, typename _void<typename Base::key_compare>::type>
    : protected Base {
 public:
    using key_type = typename Base::key_type;
    using key_compare = typename Base::key_compare;
    using value_compare = typename Base::value_compare;

    using Base::Base;

 private:
    using DefaultCompare = std::less<key_type>;
    using DefaultTransparentCompare = std::less<void>;

 public:
    static constexpr bool iterable =
        Itble == Iterable::Yes ||
        (Itble == Iterable::Auto &&  // either the comparator is different then the default (or a
                                     // transparent version thereof)
         ((!std::is_same<key_compare, DefaultCompare>::value &&
           !std::is_same<key_compare, DefaultTransparentCompare>::value)
          // or the key is not a pointer or string or tuple thereof
          || detail::is_stable<key_type>::value));
};

/// This class implements most of the wrapping of conditionally-iterable containers and is intended
/// to be used as base of the actual wrappers. It is missing the pieces that distinguish sets from
/// maps and functions that should be defined with the end type -- swap and comparison operators.
template <typename Base, Iterable Itble>
class CondIterableAssoc : public CondIterableAssocBase<Base, Itble> {
    using ImmBase = CondIterableAssocBase<Base, Itble>;

 public:
    using key_type = typename Base::key_type;
    using value_type = typename Base::value_type;
    using size_type = typename Base::size_type;
    using difference_type = typename Base::difference_type;
    using allocator_type = typename Base::allocator_type;
    using reference = typename Base::reference;
    using const_reference = typename Base::const_reference;
    using pointer = typename Base::pointer;
    using const_pointer = typename Base::const_pointer;
    using iterator = typename Base::iterator;
    using const_iterator = typename Base::const_iterator;
    using reverse_iterator = typename Base::reverse_iterator;
    using const_reverse_iterator = typename Base::const_reverse_iterator;
    // TODO(C++update): node_type, insert_retur_type

 public:
    // inherit constructors
    using ImmBase::ImmBase;

    using Base::operator=;
    using Base::get_allocator;

    iterator begin() noexcept {
        this->_ensure_iterable();
        return Base::begin();
    }
    const_iterator begin() const noexcept {
        this->_ensure_iterable();
        return Base::begin();
    }
    const_iterator cbegin() const noexcept {
        this->_ensure_iterable();
        return Base::cbegin();
    }

    iterator unstable_begin() noexcept { return Base::begin(); }
    const_iterator unstable_begin() const noexcept { return Base::begin(); }
    const_iterator unstable_cbegin() const noexcept { return Base::cbegin(); }

    using Base::cend;
    using Base::end;

    reverse_iterator rbegin() noexcept {
        this->_ensure_iterable();
        return Base::rbegin();
    }
    const_reverse_iterator rbegin() const noexcept {
        this->_ensure_iterable();
        return Base::rbegin();
    }
    const_reverse_iterator crbegin() const noexcept {
        this->_ensure_iterable();
        return Base::crbegin();
    }

    using Base::crend;
    using Base::rend;

    using Base::empty;
    using Base::max_size;
    using Base::size;

    using Base::clear;
    using Base::emplace;
    using Base::emplace_hint;
    using Base::erase;
    using Base::insert;
    using Base::swap;

    using Base::count;
    using Base::find;

    std::pair<iterator, iterator> equal_range(const key_type &key) {
        this->_ensure_iterable();
        return Base::equal_range(key);
    }
    std::pair<const iterator, const_iterator> equal_range(const key_type &key) const {
        this->_ensure_iterable();
        return Base::equal_range(key);
    }

    iterator lower_bound(const key_type &key) {
        this->_ensure_iterable();
        return Base::lower_bound(key);
    }
    const_iterator lower_bound(const key_type &key) const {
        this->_ensure_iterable();
        return Base::lower_bound(key);
    }

    iterator upper_bound(const key_type &key) {
        this->_ensure_iterable();
        return Base::upper_bound(key);
    }
    const_iterator upper_bound(const key_type &key) const {
        this->_ensure_iterable();
        return Base::upper_bound(key);
    }

    // TODO(C++update): in C++20
    bool contains(const key_type &key) const { return find(key) != end(); }

    using Base::key_comp;
    using Base::value_comp;

 protected:
    void _ensure_iterable() const {
        static_assert(ImmBase::iterable,
                      "To ensure the compiler is deterministic and preserves behaviour when "
                      "source values are renamed it is not possible to iterate over maps of "
                      "pointer and string types. Either use `ordered_map` to iterate in the "
                      "insertion order, or specify a comparator that uses some ordering that is "
                      "guaranteed to be stable accross runs and with renaming in the source file. "
                      "If you know for sure it is safe to iterate in string order in this case "
                      "(e.g. map of constant strings or other strings that do not depend on "
                      "the source code, you can use `iterable_map` instead of `map`. ");
    }
};
}  // namespace detail

/// Works just like a normal std::map, except it cannot be iterated over if the key is not stable
/// (unless explicitly overridden by the last template parameter or preferably use of iterable_map
/// or non_iterable_map).
/// \see namespace assoc
template <typename Key, typename T, typename Compare = std::less<Key>,
          typename Allocator = std::allocator<std::pair<const Key, T>>,
          Iterable Itble = Iterable::Auto>
class map : public detail::CondIterableAssoc<std::map<Key, T, Compare, Allocator>, Itble> {
    using ABase = std::map<Key, T, Compare, Allocator>;
    using Base = detail::CondIterableAssoc<ABase, Itble>;

 public:
    using mapped_type = typename Base::mapped_type;

    using Base::Base;

    // the initializer_list constructor must be explicitly specified for GCC 6
    // to pick it up
    map(std::initializer_list<typename Base::value_type> init, const Compare &comp = Compare(),
        const Allocator &alloc = Allocator())
        : Base(init, comp, alloc) {}

    // seems like GCC 6 ignores the default constructor in some cases
    // FIXME(vstill): remove when GCC 6 is no longer needed
    map() = default;

    using ABase::operator=;

    using ABase::at;
    using ABase::operator[];
    // TODO(C++update): C++17 noexcept
    void swap(map &other) { ABase::swap(other); }

    using ABase::extract;
    using ABase::insert_or_assign;
    using ABase::merge;
    using ABase::try_emplace;

    BFN_ASSOC_OP_DEF(map, ==)
    BFN_ASSOC_OP_DEF(map, !=)
    BFN_ASSOC_OP_DEF(map, <)
    BFN_ASSOC_OP_DEF(map, >)
    BFN_ASSOC_OP_DEF(map, <=)
    BFN_ASSOC_OP_DEF(map, >=)
    // TODO(C++update): C++20 <=>

    /// Extract an iterable that can be iterated over even if this container is not iterable.
    /// This is intended for targeted use at places where it is actually safe to iterate over
    /// a container with unspecified order -- i.e. in cases where the result does in no way
    /// depend on the iteration order. An example can be checking if a given value is present
    /// in the map.
    const std::map<Key, T, Compare, Allocator> &unstable_iterable() const {
        return static_cast<const ABase &>(*this);
    }
};

/// Works just like a normal std::set, except it cannot be iterated over if the key is not stable
/// (unless explicitly overridden by the last template parameter or preferably use of iterable_set
/// or non_iterable_set).
/// \see namespace assoc
template <typename Key, typename Compare = std::less<Key>, typename Allocator = std::allocator<Key>,
          Iterable Itble = Iterable::Auto>
class set : public detail::CondIterableAssoc<std::set<Key, Compare, Allocator>, Itble> {
    using ABase = std::set<Key, Compare, Allocator>;
    using Base = detail::CondIterableAssoc<ABase, Itble>;

 public:
    // Inherit constructors
    using Base::Base;

    // the initializer_list constructor must be explicitly specified for GCC 6
    // to pick it up
    set(std::initializer_list<typename Base::value_type> init, const Compare &comp = Compare(),
        const Allocator &alloc = Allocator())
        : Base(init, comp, alloc) {}

    // seems like GCC 6 ignores the default constructor in some cases
    // FIXME(vstill): remove when GCC 6 is no longer needed
    set() = default;

    using ABase::operator=;

    // TODO(C++update): C++17 noexcept
    void swap(set &other) { Base::swap(other); }

    BFN_ASSOC_OP_DEF(set, ==)
    BFN_ASSOC_OP_DEF(set, !=)
    BFN_ASSOC_OP_DEF(set, <)
    BFN_ASSOC_OP_DEF(set, >)
    BFN_ASSOC_OP_DEF(set, <=)
    BFN_ASSOC_OP_DEF(set, >=)
    // TODO(C++update): C++20 <=>

    /// Extract an iterable that can be iterated over even if this container is not iterable.
    /// This is intended for targeted use at places where it is actually safe to iterate over
    /// a container with unspecified order -- i.e. in cases where the result does in no way
    /// depend on the iteration order. An example can be checking if a given value is present
    /// in the map.
    const std::set<Key, Compare, Allocator> &unstable_iterable() const {
        return static_cast<const Base &>(*this);
    }
};

/// Map container implemented as hashtable. Provides average constant time lookup. Member
/// functions used for iteration (begin) are intentionaly not exposed to avoid accidental
/// nondeterministic behavior of the compiler.
/// \see namespace assoc
template <typename Key, typename T, typename Hash = boost::hash<Key>,
          typename Equal = std::equal_to<Key>,
          typename Allocator = std::allocator<std::pair<const Key, T>>>
class hash_map : private std::unordered_map<Key, T, Hash, Equal, Allocator> {
    using ABase = std::unordered_map<Key, T, Hash, Equal, Allocator>;

 public:
    using mapped_type = typename ABase::mapped_type;

    using ABase::ABase;
    using ABase::operator=;

    using ABase::at;
    using ABase::operator[];

    using ABase::cend;
    using ABase::end;

    using ABase::empty;
    using ABase::max_size;
    using ABase::size;

    using ABase::clear;
    using ABase::emplace;
    using ABase::emplace_hint;
    using ABase::extract;
    using ABase::insert;
    using ABase::insert_or_assign;
    using ABase::merge;
    using ABase::try_emplace;

    using ABase::erase;
    using ABase::swap;

    using ABase::count;
    using ABase::find;

    BFN_ASSOC_OP_DEF(hash_map, ==)
    BFN_ASSOC_OP_DEF(hash_map, !=)

    void swap(hash_map &other) noexcept { ABase::swap(other); }

    /// Extract an iterable that can be iterated over even if this container is not iterable.
    /// This is intended for targeted use at places where it is actually safe to iterate over
    /// a container with unspecified order -- i.e. in cases where the result does in no way
    /// depend on the iteration order. An example can be checking if a given value is present
    /// in the map.
    const ABase &unstable_iterable() const { return static_cast<const ABase &>(*this); }
};

/// Set container implemented as hashtable. Provides average constant time lookup. Member
/// functions used for iteration (begin) are intentionaly not exposed to avoid accidental
/// nondeterministic behavior of the compiler.
/// \see namespace assoc
template <typename T, typename Hash = boost::hash<T>, typename Equal = std::equal_to<T>,
          typename Allocator = std::allocator<T>>
class hash_set : private std::unordered_set<T, Hash, Equal, Allocator> {
    using ABase = std::unordered_set<T, Hash, Equal, Allocator>;

 public:
    using ABase::ABase;

    using ABase::operator=;

    using ABase::cend;
    using ABase::end;

    using ABase::empty;
    using ABase::max_size;
    using ABase::size;

    using ABase::clear;
    using ABase::emplace;
    using ABase::emplace_hint;
    using ABase::erase;
    using ABase::insert;
    using ABase::swap;

    using ABase::count;
    using ABase::find;

    BFN_ASSOC_OP_DEF(hash_set, ==)
    BFN_ASSOC_OP_DEF(hash_set, !=)

    void swap(hash_set &other) noexcept { ABase::swap(other); }

    /// Extract an iterable that can be iterated over even if this container is not iterable.
    /// This is intended for targeted use at places where it is actually safe to iterate over
    /// a container with unspecified order -- i.e. in cases where the result does in no way
    /// depend on the iteration order. An example can be checking if a given value is present
    /// in the map.
    const ABase &unstable_iterable() const { return static_cast<const ABase &>(*this); }
};

template <typename Key, typename T, typename Compare = std::less<Key>,
          typename Allocator = std::allocator<std::pair<const Key, T>>>
using iterable_map = map<Key, T, Compare, Allocator, Iterable::Yes>;

template <typename Key, typename T, typename Compare = std::less<Key>,
          typename Allocator = std::allocator<std::pair<const Key, T>>>
using non_iterable_map = map<Key, T, Compare, Allocator, Iterable::No>;

template <typename Key, typename T, typename Compare = std::less<Key>,
          typename Allocator = std::allocator<std::pair<const Key, T>>>
using ordered_map = ::P4::ordered_map<Key, T, Compare, Allocator>;

template <typename Key, typename Compare = std::less<Key>, typename Allocator = std::allocator<Key>>
using iterable_set = set<Key, Compare, Allocator, Iterable::Yes>;

template <typename Key, typename Compare = std::less<Key>, typename Allocator = std::allocator<Key>>
using non_iterable_set = set<Key, Compare, Allocator, Iterable::No>;

template <typename Key, typename Compare = std::less<Key>, typename Allocator = std::allocator<Key>>
using ordered_set = ::P4::ordered_set<Key, Compare, Allocator>;

// Note that in these functions (which correspond to the ones found in p4c/lib/map.h) the use of
// unstable_iterable is safe as the get functions are doing just lookups. This function basicaly
// extracts reference to the underlying standard container from the `assoc::` container wrapper.
template <class K, class T, class V, class Comp, class Alloc, Iterable It>
inline V get(const map<K, V, Comp, Alloc, It> &m, T key, V def = V()) {
    return get(m.unstable_iterable(), key, def);
}

template <class K, class T, class V, class Comp, class Alloc, Iterable It>
inline V *getref(map<K, V, Comp, Alloc, It> &m, T key) {
    return getref(m.unstable_iterable(), key);
}

template <class K, class T, class V, class Comp, class Alloc, Iterable It>
inline const V *getref(const map<K, V, Comp, Alloc, It> &m, T key) {
    return getref(m.unstable_iterable(), key);
}

template <class K, class T, class V, class Comp, class Alloc, Iterable It>
inline V get(const map<K, V, Comp, Alloc, It> *m, T key, V def = V()) {
    return m ? get(*m, key, def) : def;
}

template <class K, class T, class V, class Comp, class Alloc, Iterable It>
inline V *getref(map<K, V, Comp, Alloc, It> *m, T key) {
    return m ? getref(*m, key) : 0;
}

template <class K, class T, class V, class Comp, class Alloc, Iterable It>
inline const V *getref(const map<K, V, Comp, Alloc, It> *m, T key) {
    return m ? getref(*m, key) : 0;
}

template <typename T>
std::ostream &operator<<(std::ostream &out, const assoc::set<T> &set) {
    return format_container(out, set, '(', ')');
}

template <typename T>
std::ostream &operator<<(std::ostream &out, const assoc::hash_set<T> &set) {
    return format_container(out, set, '(', ')');
}

}  // namespace assoc

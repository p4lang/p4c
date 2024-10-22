/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cstring>
#include <functional>

#ifndef BF_P4C_LIB_CMP_H_
#define BF_P4C_LIB_CMP_H_

/// Provides templates for lifting comparison operators from objects to pointers, and deriving
/// related comparison operators on objects.

#define OPERATOR(op, Op)                                                                 \
    struct Op {                                                                          \
        bool operator()(const T *left, const T *right) const { return op(left, right); } \
    };                                                                                   \
    static bool op(const T *left, const T *right)

/// Lifts == on objects to functions and functors for == and != on pointers, and also derives !=
/// on objects. The virtual operator== is assumed to implement an equivalence relation.
template <class T>
class LiftEqual {
 public:
    virtual bool operator==(const T &) const = 0;

    bool operator!=(const T &other) const { return !(operator==(other)); }

    OPERATOR(equal, Equal) {
        if (left != right && left && right) return *left == *right;
        return left == right;
    }

    OPERATOR(not_equal, NotEqual) { return !equal(left, right); }
};

/// Lifts < on objects to functions and functors for ==, !=, <, >, <=, and >= on pointers, and also
/// derives ==, !=, >, <=, and >= on objects. The lifted virtual operator< is assumed to implement
/// a strict weak ordering relation.
template <class T>
class LiftLess {
 public:
    virtual bool operator<(const T &) const = 0;

    bool operator==(const T &other) const { return !operator!=(other); }

    bool operator!=(const T &other) const { return operator<(other) || operator>(other); }

    bool operator>(const T &other) const { return other.operator<(*dynamic_cast<const T *>(this)); }

    bool operator<=(const T &other) const { return !operator>(other); }
    bool operator>=(const T &other) const { return !(operator<(other)); }

    OPERATOR(not_equal, NotEqual) { return less(left, right) || greater(left, right); }

    OPERATOR(less, Less) {
        if (left != right && left && right) return *left < *right;
        return left < right;
    }

    OPERATOR(equal, Equal) { return !not_equal(left, right); }
    OPERATOR(greater, Greater) { return less(right, left); }
    OPERATOR(less_equal, LessEqual) { return !greater(left, right); }
    OPERATOR(greater_equal, GreaterEqual) { return !less(left, right); }
};

/// Similar to LiftLess, except allows subclasses to provide a more efficient ==. The virtual
/// operator== is assumed to be the equivalence relation induced by operator<.
template <class T>
class LiftCompare : public LiftEqual<T>, public LiftLess<T> {
 public:
    bool operator!=(const T &other) const { return LiftEqual<T>::operator!=(other); }

    OPERATOR(equal, Equal) { return LiftEqual<T>::equal(left, right); }

    OPERATOR(not_equal, NotEqual) { return LiftEqual<T>::not_equal(left, right); }
};

/// A type suitable as map comparator that will compare objects of type T.
/// Expects that `T::getName()` returns `IR::ID`
/// The same comparer works for `T &` - reference comparison and `T *` - pointer comparison.
template <class T>
class ByNameLess {
 public:
    bool operator()(const T &a, const T &b) const {
        return std::strcmp(a.getName().name, b.getName().name) < 0;
    }

    bool operator()(const T *a, const T *b) const {
        // note: std::less is guaranteed to be defined for pointers, operator < is not
        return a && b ? (*this)(*a, *b) : std::less<T *>()(a, b);
    }
};

/// Lifts `std::less`, or user-specified comparer to `std::pair<A, B>` using lexicographical
/// comparison.
/// Comparers given in `AComp`, `BComp` must be stateless and default constructible.
/// Comparers must produce total order, in particular it must hold that
/// `!(a < b) && !(b < a) => a == b`.
template <class A, class B, class AComp = std::less<A>, class BCmp = std::less<B>>
class PairLess {
 public:
    using Pair = std::pair<A, B>;
    bool operator()(const Pair &a, const Pair &b) const {
        AComp acmp;
        if (acmp(a.first, b.first)) return true;
        // !(a < b) && !(b < a) => a == b
        if (!acmp(b.first, a.first)) return BCmp()(a.second, b.second);
        return false;
    }
};
#endif /* BF_P4C_LIB_CMP_H_ */

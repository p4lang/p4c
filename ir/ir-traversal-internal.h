/*
Copyright 2024-present Intel Corporation.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
/**
 * \file
 * \brief IR traversing utilities, internal implementation details, not to be used directly.
 * \date 2024
 * \author Vladimír Štill
 */

#ifndef IR_IR_TRAVERSAL_INTERNAL_H_
#define IR_IR_TRAVERSAL_INTERNAL_H_

#include "lib/exceptions.h"
#include "lib/rtti_utils.h"

#ifndef IR_IR_TRAVERSAL_INTERNAL_ENABLE
#error "This file must not be used directly"
#endif

namespace P4::IR::Traversal::Detail {

/// @brief Internal, don't use directly.
/// The class exists so that the functions can be mutually recursive.
struct Traverse {
    template <typename Obj, typename T>
    static const Obj *modify(const Obj *, Assign<const T *> asgn) {
        static_assert(!std::is_const_v<Obj>, "Cannot modify constant object");
        return asgn.value;
    }

    template <typename Obj, typename T>
    static const Obj *modify(Obj *, Assign<const T *> asgn) {
        static_assert(!std::is_const_v<Obj>, "Cannot modify constant object");
        return asgn.value;
    }

    template <typename Obj, typename T>
    static Obj *modify(Obj *obj, Assign<T> &&asgn) {
        static_assert(!std::is_const_v<Obj>, "Cannot modify constant object");
        *obj = std::move(asgn.value);
        return obj;
    }

    template <typename Obj, typename T>
    static Obj *modify(Obj *obj, const Assign<T> &asgn) {
        static_assert(!std::is_const_v<Obj>, "Cannot modify constant object");
        *obj = asgn.value;
        return obj;
    }

    template <typename Obj, typename Fn>
    static decltype(auto) modify(Obj *obj, Fn fn) {
        static_assert(!std::is_const_v<Obj>, "Cannot modify constant object");
        return fn(obj);
    }

    template <typename Obj, typename... Selectors>
    static Obj *modify(Obj *obj, Index idx, Selectors &&...selectors) {
        static_assert(!std::is_const_v<Obj>, "Cannot modify constant object");
        BUG_CHECK(obj->size() > idx.value, "Index %1% out of bounds of %2%", idx.value, obj);
        modifyRef((*obj)[idx.value], std::forward<Selectors>(selectors)...);
        return obj;
    }

    template <typename Obj, typename To, typename... Selectors>
    static To *modify(Obj *obj, RTTI::Detail::ToType<To> cast, Selectors &&...selectors) {
        static_assert(!std::is_const_v<Obj>, "Cannot modify constant object");
        auto *casted = cast(obj);
        BUG_CHECK(casted, "Cast of %1% failed", obj);
        return modify(casted, std::forward<Selectors>(selectors)...);
    }

    template <typename Obj, typename T, typename Sub, typename... Selectors>
    static Obj *modify(Obj *obj, Sub T::*member, Selectors &&...selectors) {
        static_assert(!std::is_const_v<Obj>, "Cannot modify constant object");
        modifyRef(obj->*member, std::forward<Selectors>(selectors)...);
        return obj;
    }

    template <typename T, typename... Selectors>
    static void modifyRef(T &ref, Selectors &&...selectors) {
        if constexpr (std::is_pointer_v<T>) {
            ref = modify(ref->clone(), std::forward<Selectors>(selectors)...);
        } else {
            auto *res = modify(&ref, std::forward<Selectors>(selectors)...);
            if (&ref != res) {
                ref = *res;
            }
        }
    }
};

}  // namespace P4::IR::Traversal::Detail

#endif  // IR_IR_TRAVERSAL_INTERNAL_H_

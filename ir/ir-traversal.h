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
 * \brief IR traversing utilities.
 * \date 2024
 * \author Vladimír Štill
 * 
 * 
 */

#include <lib/rtti_utils.h>

namespace P4::IR::Traversal {

template<typename T>
struct Assign {
    explicit Assign(const T &value) : value(value) {}
    T value;
};

struct Index {
    explicit Index(size_t value) : value(value) {}
    size_t value;
};

namespace Detail {

// The class exists so that the functions can be mutually recursive.
struct Traverse {

    template<typename Obj, typename T>
    static const Obj *modify(const Obj *, Assign<const T *> asgn)
    {
        return asgn.value;
    }

    template<typename Obj, typename T>
    static const Obj *modify(Obj *, Assign<const T *> asgn)
    {
        return asgn.value;
    }

    template<typename Obj>
    static Obj *modify(Obj *obj, Assign<Obj> &&asgn)
    {
        *obj = std::move(asgn.value);
        return obj;
    }

    template<typename Obj>
    static Obj *modify(Obj *obj, const Assign<Obj> &asgn)
    {
        *obj = asgn.value;
        return obj;
    }

    template<typename Obj, typename Fn>
    static decltype(auto) modify(Obj *obj, Fn fn)
    {
        return fn(obj);
    }

    template<typename Obj, typename... Selectors>
    static Obj *modify(Obj *obj, Index idx, Selectors &&... selectors)
    {
        BUG_CHECK(obj->size() > idx.value, "Index %1% out of bounds of %2%", idx.value, obj);
        auto &val = (*obj)[idx.value];
        if constexpr (std::is_pointer_v<std::remove_reference_t<decltype(val)>>) {
            val = modify(val->clone(), std::forward<Selectors>(selectors)...);
        } else {
            const auto *res = modify(&val, std::forward<Selectors>(selectors)...);
            if (&val != res) {
                val = *res;
            }
        }
        return obj;
    }

    template<typename Obj, typename To, typename... Selectors>
    static To *modify(Obj *obj, RTTI::Detail::ToType<To> cast, Selectors &&... selectors)
    {
        return modify(cast(obj), std::forward<Selectors>(selectors)...);
    }

    template<typename Obj, typename T, typename Sub, typename... Selectors>
    static Obj *modify(Obj *obj, Sub T::* member, Selectors &&... selectors)
    {
        static_assert(!std::is_const_v<Obj>, "Cannot modify constant object");
        if constexpr (std::is_pointer_v<Sub>) {
            obj->*member = modify((obj->*member)->clone(), std::forward<Selectors>(selectors)...);
        } else {
            const auto *res = modify(&(obj->*member), std::forward<Selectors>(selectors)...);
            if (&(obj->*member) != res) {
                obj->*member = *res;
            }
        }
        return obj;
    }
};

} // namespace Details

/// @brief 
/// @param obj           An object to modify.
/// @param ...selectors  A series of selectors ending either with Assign, or with a modifyier
///                      function (typically a lambda).
/// @return  Modified object. This will be the same object as @p obj.
template<typename Obj, typename... Selectors>
Obj *modify(Obj *obj, Selectors &&... selectors) {
    return Detail::Traverse::modify(obj, std::forward<Selectors>(selectors)...);
}

/// @brief Similar to modify, but accepts constant argument which is cloned. Therefore, the result
/// is a different object then @p obj.
template<typename Obj, typename... Selectors>
Obj *apply(const Obj *obj, Selectors &&... selectors) {
    return Detail::Traverse::modify(obj->clone(), std::forward<Selectors>(selectors)...);
}


}  // namespace P4::IR::Traversal

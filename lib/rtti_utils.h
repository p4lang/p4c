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

/// @file
/// @brief Utilities that help with use of custom-RTTI-enabled classes.

#ifndef LIB_RTTI_UTILS_H_
#define LIB_RTTI_UTILS_H_

#include <type_traits>

#include "rtti.h"

namespace p4c::RTTI {

/// A trait that check T is custom-RTTI-enabled. Works just like standard property type traits.
/// One would normally use the _v variant.
/// NOTE: Custom-RTTI-enabled classes should not only derive from RTTI::Base, but should also
/// declare typeId properly. However, not doing so would be a bug and use of such class in
/// RTTI-related operations would lead to static_assert failuire in RTTI::TypeInfo, so we just check
/// the base.
template <typename T>
struct has_rtti : std::is_base_of<RTTI::Base, T> {};

/// A trait that check T is custom-RTTI-enabled, variable version. Works just like standard property
/// type traits.
template <typename T>
inline constexpr const bool has_rtti_v = has_rtti<T>::value;

/// A type trait that check that all the types are custom-RTTI-enabled.
template <typename... Ts>
inline constexpr const bool all_have_rtti_v = (has_rtti_v<Ts> && ...);

/// A type condition for checking that T is custom-RTTI-enabled. A specialization of std::enable_if
/// and can be used in the same way.
/// @tparam T  The type to be checked.
/// @tparam R  The type that will be used for the ::type member if T is custom-rtti-enabled.
///            Defaults to void.
template <typename T, typename R = void>
struct enable_if_has_rtti : std::enable_if<has_rtti_v<T>, R> {};

/// A type condition for checking that all the types have RTTI. The resulting type is either not
/// defined if some of the types does not have RTTI (so a SFINAE use will fail), or it is defined to
/// be void. If you need another result type, please use std::enable_if<all_have_rtti_v<...>, T>.
template <typename... Ts>
using enable_if_all_have_rtti_t = std::enable_if_t<all_have_rtti_v<Ts...>, void>;

/// A type condition for checking that T is custom-RTTI-enabled. A specialization of
/// std::enable_if_t and can be used in the same way.
/// @tparam T  The type to be checked.
/// @tparam R  The type that will be used for the ::type member if T is custom-rtti-enabled.
///            Defaults to void.
template <typename T, typename R = void>
using enable_if_has_rtti_t = typename enable_if_has_rtti<T, R>::type;

namespace Detail {

template <typename To, typename = enable_if_has_rtti_t<To>>
struct ToType {
    template <typename From, typename = enable_if_has_rtti_t<From>>
    To *operator()(From *obj) const {
        return obj ? obj->template to<To>() : nullptr;
    }

    template <typename From, typename = enable_if_has_rtti_t<From>>
    const To *operator()(const From *obj) const {
        return obj ? obj->template to<To>() : nullptr;
    }
};

template <typename... Targets>
// TODO(C++20): use concepts to check enable_if_all_have_rtti_t<Targets...>> & that there is at
// least 1 target type.
struct IsType {
    static_assert(sizeof...(Targets) > 0,
                  "At least one target type needs to be given for RTTI::is");
    static_assert(all_have_rtti_v<Targets...>,
                  "All types in RTTI::is<Ts> need to be custom-rtti-enabled");

    template <typename From, typename = enable_if_has_rtti_t<From>>
    bool operator()(const From *obj) const {
        return obj && (obj->template is<Targets>() || ...);
    }
};

}  // namespace Detail

/// A freestanding wrapper over From::to<T>(). It is an object, but is intended to be used a
/// function (similarly to C++20 range-implementing objects). Can be passed to functions expecting a
/// callable.
/// The callable has following signatures:
/// template<typename From, typename = enable_if_has_rtti_t<From>
/// *   To *(From *)
/// *   const To *(const From *)
///
/// Examples:
/// *   RTTI::to<IR::Type>(n)
/// *   std::copy_if(a.begin(), a.end(), std::back_inserter(b), RTTI::to<IR::Declaration>);
///
/// @tparam To  The target type to cast to.
/// @returns nullptr if the from value is null or not of the To type, otherwise from cast to
/// To-type.
template <typename To>
inline const Detail::ToType<To> to;

/// A freestanding wrapper over From::is<T>(). Can be used to check if the value is of the
/// given type. A callable object with behaving as if it had the following signature:
/// template<typename From, typename = enable_if_has_rtti_t<From>
/// bool (From *)
///
/// Examples:
/// *   if (RTTI::is<IR::Type::Bits>(typeOrNull)) { ... }
/// *   std::find(x.begin(), x.end(), RTTI::is<IR::Declaration>);
///
/// @tparam  Target  The target type, from is checked to be of this type.
/// @returns true if the from object is not null and is of the target type, false otherwise.
template <typename Target>
inline const Detail::IsType<Target> is;

/// Similar to @ref RTTI::is, but accept accepts multiple types and succeeds if any of them matches
/// the type of the object.
///
/// Examples:
/// *   if (RTTI::isAny<IR::Type::Bits>(typeOrNull)) { ... }
/// *   std::find(x.begin(), x.end(), RTTI::isAny<IR::Declaration, IR::Type_Declaration>);
///
/// @tparam  Targets  The target type, from is checked to be of one of these types.
/// @returns true if the from object is not null and is of any of the target types, false otherwise.
template <typename... Targets>
inline const Detail::IsType<Targets...> isAny;

}  // namespace p4c::RTTI

#endif  // LIB_RTTI_UTILS_H_

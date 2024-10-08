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
 * \brief IR traversing utilities. This is mainly useful for modifying a sub-component of an IR
 * node.
 * \date 2024
 * \author Vladimír Štill
 */

#ifndef IR_IR_TRAVERSAL_H_
#define IR_IR_TRAVERSAL_H_
#define IR_IR_TRAVERSAL_INTERNAL_ENABLE

#include <cstddef>

namespace P4::IR::Traversal {

/// @brief A selector used at the end of selector chain to assign to the current sub-object.
/// e.g. `modify(obj, &IR::AssignmentStatement::left, Assign(var))` will set the LHS of assignment.
/// @tparam T the parameter is usually derived by the C++ compiler.
template <typename T>
struct Assign {
    explicit Assign(const T &value) : value(value) {}
    T value;
};

/// @brief Select an index of a integer-indexed sub-object. %This is useful e.g. to select first
/// parameter of a method call and in similar cases involving @ref IR::Vector or @ref
/// IR::IndexedVector.
///
/// @code
/// modify(mce, &IR::MethodCallExpression::arguments, Index(0), &IR::Argument::expression,
/// Assign(val))
/// @endcode
/// This snippet assigns `val` into the first argument of a method call.
struct Index {
    explicit Index(size_t value) : value(value) {}
    size_t value;
};

}  // namespace P4::IR::Traversal

// Put the internals out of the main file to increase its readability.
#include "ir-traversal-internal.h"

namespace P4::IR::Traversal {

/// @brief Given an object @p obj and a series of selector (ending with a modifier), modify the @p
/// obj's sub object at the selected path. %This is useful for deeper modification of objects that
/// is not based on object types (in that case please use visitors) but on object structure (e.g.
/// member paths of C++ objects).
/// @param obj           An object to modify.
/// @param ...selectors  A series of selectors ending either with Assign, or with a modifier
///                      function (typically a lambda).
/// @return  Modified object. This will usually be the same object as @p obj (unless the only
/// selector is Assign with a pointer).
///
/// @section sec_selectors Path Selectors
///
/// Selection of the sub-objects is perfomed mainly using member pointers, but other selectors are
/// also possible.
///
/// - Object member pointer (e.g. `&IR::Expression::type`, `&IR::AssignmentStatement::left`) --
///   these are used to select members of object. Note that since the IR sometimes uses inline
///   members and sometimes uses pointer members these are transparently handled the same -- this
///   behaves as-if at any point the current object was a pointer to value that is to be processed
///   by the following selectors.
/// - @ref `IR::Traversal::Index` -- used for indexing e.g. @ref `IR::Vector` or @ref
///   `IR::IndexedVector`.
/// - @ref `RTTI::to<T>` -- used to cast to a more specific type. This is necessary to access
///   members of the more specific type (e.g. if you know that a RHS of `IR::AssignmentStatement` is
///   `IR::Lss`, you can use `RTTI::to<IR::Lss>` and then access members of `IR::Lss`. Note that the
///   cast is not applied to anything -- we are passing a cast object.
///
/// @section sec_modifiers Modifiers
///
/// Modifier is a last component of the chain, it modifies the selected object.
/// - @ref `Traversal::Assign` -- a simple modifier that assigns/replaces the currently selected
///   value. If the argument is a pointer, the value is replaced, if the argument is non-pointer,
///   the selected value is assigned.
/// - freeform modifier object -- usually a lambda. This object will receive pointer to non-const
///   value of the currently selected object and can modify it in any way (e.g. increment value).
///
/// For example, given an extern call, you can modify the type of the first argument by adding a
/// cast to it like this:
/// @code
/// modify(call, &IR::MethodCallStatement::methodCall, &IR::MethodCallExpression::arguments,
///        Index(0), &IR::Argument::expression, [&](const IR::Expression *expr)
///        {
///            return new IR::Cast(expr->srcInfo, idxType, expr);
///        });
/// @endcode
///
/// Similarly, you can set the type of the first argument in the type of the extern like this:
/// @code
/// modify(call, &IR::MethodCallStatement::methodCall, &IR::MethodCallExpression::method,
///        &IR::Expression::type, RTTI::to<IR::Type_Method>, &IR::Type_Method::parameters,
///        &IR::ParameterList::parameters, Index(0), &IR::Parameter::type, Assign(idxType));
/// @endcode
///
/// Any time a cast fails or an index is out of range the modification triggers a `BUG`.
///
/// Please note that any modification is applied only to the selected path, that is, if the same
/// node occurs multiple times in the IR DAG, and one of them is "focused" by the selector and
/// modified, the other instance is unchanged (because the "focused" instance is cloned).
template <typename Obj, typename... Selectors>
Obj *modify(Obj *obj, Selectors &&...selectors) {
    return Detail::Traverse::modify(obj, std::forward<Selectors>(selectors)...);
}

/// @brief Similar to modify, but accepts constant argument which is cloned. Therefore, the result
/// is a different object then @p obj. @sa `Traversal::modify`.
template <typename Obj, typename... Selectors>
Obj *apply(const Obj *obj, Selectors &&...selectors) {
    return Detail::Traverse::modify(obj->clone(), std::forward<Selectors>(selectors)...);
}

}  // namespace P4::IR::Traversal

#undef IR_IR_TRAVERSAL_INTERNAL_ENABLE
#endif  // IR_IR_TRAVERSAL_H_

/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 * Copyright 2022-present VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_CASTABLE_H_
#define LIB_CASTABLE_H_

#include <typeinfo>

#include "lib/exceptions.h"
#include "lib/rtti.h"

namespace P4 {

/// Handy type conversion methods that can be inherited by various base classes.
/// In order to use ICastable one also need to provide intrusive lightweight RTTI
/// metadata for the given class hierarchy. See `docs/C++.md` for more information,
/// but in short, one need to use DECLARE_TYPEINFO() macro for this.
/// There is no DECLARE_TYPEINFO for ICastable itself as we are not expecting
/// pointers neither to ICastable nor to RTTI::Base to appear within the codebase.
/// As the same time, one should not cast to ICastable as well (via e.g.
/// `->to<ICastable>()`. Use DECLARE_TYPEINFO without bases to specify the base class
/// for a given herarchy.
class ICastable : public virtual RTTI::Base {
 public:
    virtual ~ICastable() = default;

    /// Tries to convert the class to type T. A BUG occurs if the cast fails.
    template <typename T>
    const T &as() const {
        return *checkedTo<T>();
    }

    /// Tries to convert the class to type T. A BUG occurs if the cast fails.
    template <typename T>
    T &as() {
        return *checkedTo<T>();
    }

    /// Performs a checked cast. A BUG occurs if the cast fails.
    template <typename T>
    const T *checkedTo() const {
        auto *result = to<T>();
        BUG_CHECK(result, "Cast failed: %1% is not a %2%", this, typeid(T).name());
        return result;
    }

    /// Performs a checked cast. A BUG occurs if the cast fails.
    template <typename T>
    T *checkedTo() {
        auto *result = to<T>();
        BUG_CHECK(result, "Cast failed: %1% is not a %2%", this, typeid(T).name());
        return result;
    }
};

}  // namespace P4

#endif /* LIB_CASTABLE_H_ */

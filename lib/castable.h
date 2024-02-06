/*
Copyright 2022-present VMware, Inc.

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

#ifndef LIB_CASTABLE_H_
#define LIB_CASTABLE_H_

#include <typeinfo>

#include "lib/exceptions.h"
#include "lib/rtti.h"

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

#endif /* LIB_CASTABLE_H_ */

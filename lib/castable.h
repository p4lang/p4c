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

/// Handy type conversion methods that can be inherited by various base classes.
class ICastable {
 public:
    virtual ~ICastable() = default;

    /// Checks whether the class is of type T. Returns true if this is the case.
    template <typename T>
    bool is() const {
        return to<T>() != nullptr;
    }

    /// Tries to convert the class to type T. Returns a nullptr if the cast fails.
    template <typename T>
    const T &as() const {
        return dynamic_cast<const T &>(*this);
    }

    /// Tries to convert the class to type T. Returns a nullptr if the cast fails.
    template <typename T>
    T *to() {
        return dynamic_cast<T *>(this);
    }

    /// Tries to convert the class to type T. Returns a nullptr if the cast fails.
    template <typename T>
    const T *to() const {
        return dynamic_cast<const T *>(this);
    }

    /// Performs a checked cast. A BUG occurs if the cast fails.
    template <typename T>
    const T *checkedTo() const {
        auto *result = to<T>();
        BUG_CHECK(result, "Cast failed: %1% is not a %2%", this, typeid(T).name());
        return result;
    }
};

#endif /* LIB_CASTABLE_H_ */

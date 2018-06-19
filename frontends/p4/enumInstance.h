/*
Copyright 2013-present Barefoot Networks, Inc.

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

#ifndef _FRONTENDS_P4_ENUMINSTANCE_H_
#define _FRONTENDS_P4_ENUMINSTANCE_H_

#include "ir/ir.h"
#include "methodInstance.h"
#include "frontends/p4/typeMap.h"

namespace P4 {

// helps resolving references to compile-time enum fields, e.g., X.a
class EnumInstance : public InstanceBase {
 protected:
    EnumInstance(const IR::ID name, const IR::Type* type): name(name), type(type) {}

 public:
    const IR::ID         name;
    const IR::Type*      type;

    /// Returns nullptr if the expression is not a compile-time constant
    /// referring to an enum
    static EnumInstance* resolve(const IR::Expression* expression, const P4::TypeMap* typeMap);
};

/// An instance of a simple enum, e.g., X.A from enum X { A, B }
class SimpleEnumInstance : public EnumInstance {
 public:
    SimpleEnumInstance(const IR::Type_Enum* type, const IR::ID name) :
            EnumInstance(name, type) {}
};

/// An instance of a serializable enum, e.g.,
/// X.A from enum bit<5> X { A = 3, B = 2 }
class SerEnumInstance : public EnumInstance {
 public:
    const IR::Expression* value;
    SerEnumInstance(const IR::Type_SerEnum* type, const IR::ID name, const IR::Expression* value) :
            EnumInstance(name, type), value(value) {}
};

}  // namespace P4

#endif /* _FRONTENDS_P4_ENUMINSTANCE_H_ */

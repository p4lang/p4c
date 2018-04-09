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

#ifndef P4C_FRONTENDS_COMMON_MODEL_H_
#define P4C_FRONTENDS_COMMON_MODEL_H_

#include "lib/cstring.h"
#include "ir/id.h"

// Classes for representing various P4 program models inside the compiler

namespace Model {

// Model element
struct Elem {
    explicit Elem(cstring name) : name(name) {}
    Elem() = delete;

    cstring name;
    IR::ID Id() const { return IR::ID(name); }
    IR::ID Id(Util::SourceInfo srcInfo) const { return IR::ID(srcInfo, name); }
    const char* str() const { return name.c_str(); }
    cstring toString() const { return name; }
};

struct Type_Model : public Elem {
    explicit Type_Model(cstring name) : Elem(name) {}
};

/// Enum_Model : Type_Model
struct Enum_Model : public Type_Model {
    explicit Enum_Model(cstring name) : Type_Model(name) {}
};

/// Extern_Model : Type_Model
struct Extern_Model : public Type_Model {
    explicit Extern_Model(cstring name) : Type_Model(name) {}
};

/// Param_Model : Elem
struct Param_Model : public Elem {
    Type_Model type;
    unsigned   index;
    Param_Model(cstring name, Type_Model type, unsigned index) :
            Elem(name), type(type), index(index) {}
};

class Model {
 public:
    cstring version;
    explicit Model(cstring version) : version(version) {}
};

}  // namespace Model

#endif /* P4C_FRONTENDS_COMMON_MODEL_H_ */

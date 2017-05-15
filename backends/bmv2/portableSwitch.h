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

#ifndef _BACKENDS_BMV2_PORTABLESWITCH_H_
#define _BACKENDS_BMV2_PORTABLESWITCH_H_

#include "ir/ir.h"
#include "lower.h"
#include "lib/gmputil.h"
#include "lib/json.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/p4/enumInstance.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/typeMap.h"
#include "helpers.h"

namespace P4 {

using ::Model::Elem;
using ::Model::Type_Model;
using ::Model::Param_Model;

// Block has a name and a collection of elements
template<typename T>
struct Block_Model : public Type_Model {
    std::vector<T> elems;
    explicit Block_Model(cstring name) :
        ::Model::Type_Model(name) {}
};

/// Enum_Model : Block_Model<Elem> : Type_Model
struct Enum_Model : public Block_Model<Elem> {
    ::Model::Type_Model type;
    explicit Enum_Model(cstring name) :
        Block_Model(name), type("Enum") {}
};

/// Parser_Model : Block_Model<Param_Model> : Type_Model
struct Parser_Model : public Block_Model<Param_Model> {
    ::Model::Type_Model type;
    explicit Parser_Model(cstring name) :
        Block_Model<Param_Model>(name), type("Parser") {}
};

/// Control_Model : Block_Model<Param_Model> : Type_Model
struct Control_Model : public Block_Model<Param_Model> {
    ::Model::Type_Model type;
    explicit Control_Model(cstring name) :
        Block_Model<Param_Model>(name), type("Control") {}
};

/// Method_Model : Block_Model<Param_Model> : Type_Model
struct Method_Model : public Block_Model<Param_Model> {
    ::Model::Type_Model type;
    explicit Method_Model(cstring name) :
        Block_Model<Param_Model>(name), type("Method") {}
};

/// Extern_Model : Block_Model<Method_Model> : Type_Model
struct Extern_Model : public Block_Model<Method_Model> {
    ::Model::Type_Model type;
    explicit Extern_Model(cstring name) :
        Block_Model<Method_Model>(name), type("Extern") {}
};

/// PortableModel : Model::Model
class PortableModel : public ::Model::Model {
 public:
    std::vector<Parser_Model*>  parsers;
    std::vector<Control_Model*> controls;
    std::vector<Extern_Model*>  externs;
    std::vector<Type_Model*>    match_kinds;
    bool find_match_kind(cstring kind_name);
    bool find_extern(cstring extern_name);
    static PortableModel instance;
    PortableModel() : ::Model::Model("0.2") {}
};

}  // namespace P4

std::ostream& operator<<(std::ostream &out, Model::Type_Model& m);
std::ostream& operator<<(std::ostream &out, Model::Param_Model& p);
std::ostream& operator<<(std::ostream &out, P4::PortableModel& e);
std::ostream& operator<<(std::ostream &out, P4::Method_Model& p);
std::ostream& operator<<(std::ostream &out, P4::Parser_Model* p);
std::ostream& operator<<(std::ostream &out, P4::Control_Model* p);
std::ostream& operator<<(std::ostream &out, P4::Extern_Model* p);

// portableSwitch

#endif  /* _BACKENDS_BMV2_PORTABLESWITCH_H_ */

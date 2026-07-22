/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef FRONTENDS_COMMON_MODEL_H_
#define FRONTENDS_COMMON_MODEL_H_

#include "ir/id.h"
#include "lib/cstring.h"

// Classes for representing various P4 program models inside the compiler

namespace P4::Model {

// Model element
struct Elem {
    explicit Elem(cstring name) : name(name) {}
    Elem() = delete;

    cstring name;
    IR::ID Id() const { return IR::ID(name); }
    IR::ID Id(Util::SourceInfo srcInfo) const { return IR::ID(srcInfo, name); }
    IR::ID Id(Util::SourceInfo srcInfo, cstring originalName) const {
        return IR::ID(srcInfo, name, originalName);
    }
    const char *str() const { return name.c_str(); }
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
    const Type_Model type;
    const unsigned index;
    Param_Model(cstring name, Type_Model type, unsigned index)
        : Elem(name), type(type), index(index) {}
};

class Model {};

}  // namespace P4::Model

#endif /* FRONTENDS_COMMON_MODEL_H_ */

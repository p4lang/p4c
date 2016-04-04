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
    const char* str() const { return name.c_str(); }
    cstring toString() const { return name; }
};

struct Type_Model : public Elem {
    explicit Type_Model(cstring name) : Elem(name) {}
};

struct Enum_Model : public Type_Model {
    explicit Enum_Model(cstring name) : Type_Model(name) {}
};

struct Extern_Model : public Type_Model {
    explicit Extern_Model(cstring name) : Type_Model(name) {}
};

struct Param_Model : public Elem {
    Type_Model type;
    Param_Model(cstring name, Type_Model type) : Elem(name), type(type) {}
};

class Model {
 public:
    cstring version;
    explicit Model(cstring version) : version(version) {}
};

}  // namespace Model

#endif /* P4C_FRONTENDS_COMMON_MODEL_H_ */

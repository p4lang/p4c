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

#include "frontends/common/model.h"
#include "portableSwitch.h"

namespace P4 {

PortableModel PortableModel::instance;

}

bool P4::PortableModel::find_match_kind(cstring kind_name) {
    bool found = false;
    for (auto m : instance.match_kinds) {
        if (m->toString() == kind_name) {
            found = true;
            break;
        }
    }
    return found;
}

bool P4::PortableModel::find_extern(cstring extern_name) {
    bool found = false;
    for (auto m : instance.externs) {
        if (m->type.toString() == extern_name) {
            found = true;
            break;
        }
    }
    return found;
}

std::ostream& operator<<(std::ostream &out, Model::Type_Model& m) {
    out << "Type_Model(" << m.toString() << ")";
    return out;
}

std::ostream& operator<<(std::ostream &out, Model::Param_Model& p) {
    out << "Param_Model(" << p.toString() << ") " << p.type;
    return out;
}

std::ostream& operator<<(std::ostream &out, P4::PortableModel& e) {
    out << "PortableModel " << e.version << std::endl;
    for (auto v : e.parsers)  out << v;
    for (auto v : e.controls) out << v;
    for (auto v : e.externs)  out << v;
    return out;
}

std::ostream& operator<<(std::ostream &out, P4::Method_Model& p) {
    out << "Method_Model(" << p.toString() << ") " << p.type << std::endl;
    for (auto e : p.elems) {
        out << "    " << e << std::endl;
    }
    return out;
}

std::ostream& operator<<(std::ostream &out, P4::Parser_Model* p) {
    out << "Parser_Model(" << p->toString() << ") " << p->type << std::endl;
    for (auto e : p->elems) {
        out << "  " << e << std::endl;
    }
    return out;
}

std::ostream& operator<<(std::ostream &out, P4::Control_Model* p) {
    out << "Control_Model(" << p->toString() << ") " << p->type << std::endl;
    for (auto e : p->elems) {
        out << "  " << e << std::endl;
    }
    return out;
}

std::ostream& operator<<(std::ostream &out, P4::Extern_Model* p) {
    out << "Extern_Model(" << p->toString() << ") " << p->type << std::endl;
    for (auto e : p->elems) {
        out << "  " << e << std::endl;
    }
    return out;
}

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

#ifndef IR_DUMP_H_
#define IR_DUMP_H_

#include <cstdint>
#include <iostream>
#include <string>

#include "ir/node.h"
#include "ir/visitor.h"

namespace P4 {

/* overloads rather than optional arguments to make it easier to call from the debugger */
void dump(std::ostream &out, const IR::Node *n);
void dump(std::ostream &out, const IR::Node *n, unsigned maxdepth);
void dump(const IR::Node *n);
void dump(const IR::Node *n, unsigned maxdepth);
void dump(const IR::INode *n);
void dump(const IR::INode *n, unsigned maxdepth);
void dump(uintptr_t p);
void dump(uintptr_t p, unsigned maxdepth);
void dump_notype(const IR::Node *n);
void dump_notype(const IR::Node *n, unsigned maxdepth);
void dump_notype(const IR::INode *n);
void dump_notype(const IR::INode *n, unsigned maxdepth);
void dump_notype(uintptr_t p);
void dump_notype(uintptr_t p, unsigned maxdepth);
void dump_src(const IR::Node *n);
void dump_src(const IR::Node *n, unsigned maxdepth);
void dump_src(const IR::INode *n);
void dump_src(const IR::INode *n, unsigned maxdepth);
void dump(std::ostream &, const Visitor::Context *);
void dump(const Visitor::Context *);

std::string dumpToString(const IR::Node *n);

class Dump {
    const IR::Node *n = nullptr;
    const Visitor::Context *ctxt = nullptr;
    unsigned maxdepth;
    friend std::ostream &operator<<(std::ostream &, const Dump &);

 public:
    explicit Dump(const IR::Node *n, unsigned maxdepth = ~0U) : n(n), maxdepth(maxdepth) {}
    explicit Dump(const Visitor::Context *ctxt) : ctxt(ctxt) {}
};

inline std::ostream &operator<<(std::ostream &out, const Dump &d) {
    if (d.n)
        dump(out, d.n, d.maxdepth);
    else
        dump(out, d.ctxt);
    return out;
}

struct DumpPipe : public Inspector {
    const char *heading;
    DumpPipe() : heading(nullptr) {}
    explicit DumpPipe(const char *h) : heading(h) {}
    bool preorder(const IR::Node *pipe) override;
};

}  // namespace P4

#endif /* IR_DUMP_H_ */

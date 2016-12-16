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

#ifndef _IR_DUMP_H_
#define _IR_DUMP_H_

#include <string>
#include <iostream>

/* overloads rather than optional arguments to make it easier to call from the debugger */
void dump(std::ostream &out, const IR::Node *n);
void dump(std::ostream &out, const IR::Node *n, unsigned maxdepth);
void dump(const IR::Node *n);
void dump(const IR::Node *n, unsigned maxdepth);
void dump(uintptr_t p);
void dump(uintptr_t p, unsigned maxdepth);
void dump_notype(const IR::Node *n);
void dump_notype(const IR::Node *n, unsigned maxdepth);
void dump_notype(uintptr_t p);
void dump_notype(uintptr_t p, unsigned maxdepth);

std::string dumpToString(const IR::Node* n);

class Dump {
    const IR::Node *n;
    unsigned maxdepth;
    friend std::ostream &operator<<(std::ostream &, const Dump &);
 public:
    explicit Dump(const IR::Node *n, unsigned maxdepth = ~0U) : n(n), maxdepth(maxdepth) {}
};

inline std::ostream &operator<<(std::ostream &out, const Dump &d) {
    dump(out, d.n, d.maxdepth);
    return out; }

#endif /* _IR_DUMP_H_ */

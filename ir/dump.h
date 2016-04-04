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
std::string dumpToString(const IR::Node* n);

class Dump {
    const IR::Node *n;
    unsigned maxdepth;
    friend std::ostream &operator<<(std::ostream &, const Dump &);
 public:
    Dump(const IR::Node *n, unsigned maxdepth = ~0U) : n(n), maxdepth(maxdepth) {}
};

inline std::ostream &operator<<(std::ostream &out, const Dump &d) {
    dump(out, d.n, d.maxdepth);
    return out; }

#endif /* _IR_DUMP_H_ */

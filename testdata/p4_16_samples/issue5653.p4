#include <core.p4>

header h_t { bit<8> f; }
struct meta_t {
    h_t h;
    bit<8> f;
}

control C(inout meta_t meta) {
    action a() {
        meta.h = {#};                          // assignment, no cast
        if (meta.h == {#}) meta.f = 1;         // equality, no cast
        if (meta.h != {#}) meta.f = 2;         // inequality, no cast
        meta.h = {#};
        if (!meta.h.isValid()) meta.f = 3;     // use !isValid() instead of isInvalid()
    }
    table t { actions = { a; } }
    apply { t.apply(); }
}

control proto(inout meta_t meta);
package top(proto p);
top(C()) main;

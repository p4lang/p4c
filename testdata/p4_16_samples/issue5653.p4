/*
 * SPDX-FileCopyrightText: 2026
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Regression test for issue #5653: comparison with {#} literal fails to compile.
 */

#include <core.p4>

header h_t { bit<8> f; }
header h2_t { bit<16> g; }
header_union hu_t { h_t a; h2_t b; }

struct meta_t {
    h_t h;
    bit<8> f;
}

struct nested_t {
    meta_t m;
}

control C(inout meta_t meta, inout nested_t nested, inout hu_t hu) {
    action a() {
        if (meta.h == {#})
            meta.f = 1;
        if (meta.h != {#})
            meta.f = 2;
        meta.h = {#};
        if (!meta.h.isValid())
            meta.f = 3;
        h_t hcast = (h_t){#};
        if (hcast == (h_t){#}) {}
        if (hu == {#}) {}
        if (hu != {#}) {}
        hu = {#};
        if (nested.m.h == {#})
            nested.m.f = 4;
    }

    table t {
        actions = { a; }
        default_action = a;
    }

    apply {
        t.apply();
    }
}

parser P(inout meta_t meta, inout nested_t nested, inout hu_t hu) {
    state start {
        meta.h = {#};
        if (meta.h == {#}) {
            meta.h.setInvalid();
        }
        transition accept;
    }
}

control proto(inout meta_t meta, inout nested_t nested, inout hu_t hu);
parser par(inout meta_t meta, inout nested_t nested, inout hu_t hu);
package top(par p, proto c);

top(P(), C()) main;

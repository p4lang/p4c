/*
 * SPDX-FileCopyrightText: 2026 aeron-gh
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// An 'error' key is non-numeric, so 'exact' and 'optional' are legal match kinds
// for it. None of the keys below should be rejected by the check added for
// https://github.com/p4lang/p4c/issues/5637

#include <core.p4>

match_kind {
    optional
}

struct meta_t {
    error e1;
    error e2;
}

control C(inout meta_t m);
package top(C c);

control MyControl(inout meta_t m) {
    action noop() {}

    table t_exact {
        key = { m.e1 : exact; }
        actions = { noop; }
    }
    table t_optional {
        key = { m.e2 : optional; }
        actions = { noop; }
    }

    apply {
        t_exact.apply();
        t_optional.apply();
    }
}

top(MyControl()) main;

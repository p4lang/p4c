/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

struct S<T> {
    T t;
}

extern E {
    E(list<S<bit<32>>> data);
    void run();
}

control c() {
    E((list<S<bit<32>>>){ {10}, {5} }) e;
    apply {
        e.run();
    }
}

control C();
package top(C _c);

top(c()) main;

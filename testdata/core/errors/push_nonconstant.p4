/*
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

header H { bit<32> u; }

control c() {
    apply {
        H[3] h;
        bit<32> x = 1;

        h.push_front(x);
    }
}

control proto();
package top(proto _p);

top(c()) main;

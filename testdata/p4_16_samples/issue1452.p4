/*
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control c() {
    bit<32> x;

    action a(inout bit<32> arg) {
        arg = 1;
        return;
    }
    apply {
        a(x);
    }
}

control proto();
package top(proto p);

top(c()) main;

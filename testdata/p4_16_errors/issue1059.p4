/*
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct X {}

control c() {
    action a(in bit<32> z) {
    }

    apply {
        X x;
        a(x.x);
    }
}

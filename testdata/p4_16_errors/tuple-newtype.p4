/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

header H { bit<32> b; }

type tuple<bit> T;

control c(out bit<32> x) {

    apply {
        T tt = (T){1};
        T tt1 = (T){0};
        T tt2 = (T){1w0};
        tt1 = tt2;
        x = 0;
    }
}

control e(out bit<32> x);
package top(e _e);

top(c()) main;

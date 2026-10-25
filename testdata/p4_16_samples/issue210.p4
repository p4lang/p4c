/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

control Ing(out bit<32> a) {
    bool b;

    action cond() {
        if (b)  // <<< use of b
           a = 5;
        else
           a = 10;
    }

    apply {
        b = true;  // <<< incorrectly removed: issue #210
        cond();
    }
}

control s(out bit<32> a);
package top(s e);

top(Ing()) main;

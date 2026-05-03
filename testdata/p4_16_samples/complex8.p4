/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

extern bit<32> f(in bit<32> x);

parser p() {
    state start {
        transition select (f(2)) {
            0:       accept;
            default: reject;
        }
    }
}

parser simple();
package top(simple e);
top(p()) main;

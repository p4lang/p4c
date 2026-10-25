/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

parser p() {
    state start {
        bit<8> x = 5;
        transition select(x, x, {x, x}, x) {
            (0, 0, {0, 0}, 0): accept;
            (1, 1, default, 1): accept;
            default: reject;
        }
    }
}

parser s();
package top(s _s);

top(p()) main;

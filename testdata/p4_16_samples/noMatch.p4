/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

parser p() {
    state start {
        bit<32> x;
        transition select(x) {
            0: reject;
        }
    }
}

parser e();
package top(e e);

top(p()) main;

/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control c(in bit<32> x) {
    apply {
        x = 3;  // x is not a left-value
    }
}

control proto(in bit<32> x);
package top(proto _p);

top(c()) main;

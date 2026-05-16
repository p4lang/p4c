/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */


control c(out bit<32> x);
package top(c _c);

control my(out bit<32> x) {
    apply {
        x = 1;
        x = 2;
    }
}

top(my()) main;

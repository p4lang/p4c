/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control c() {
    bit<32> a;
    bit<32> b;
    apply {
        { a, b } = { 10, 20 };
    }
}

control proto();
package top(proto _p);

top(c()) main;

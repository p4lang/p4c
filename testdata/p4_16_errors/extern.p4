/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern X {}

control c(in X x) { // Cannot have externs with direction
    apply {
    }
}

control proto(in X x);
package top(proto _c);

top(c()) main;

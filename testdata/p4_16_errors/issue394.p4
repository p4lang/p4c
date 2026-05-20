/*
 * Copyright 2017 VMware, Inc.
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
extern C { bool get(); }

control X(out bool b) {
    C c;
    apply { b = c.get(); }
}

control Z(out bool a);
package top(Z z);

top(X()) main;

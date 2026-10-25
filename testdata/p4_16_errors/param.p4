/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
extern E {
    E();
    void call();
}

control c() {
    action a(E e) { e.call(); }
    E() einst;
    apply {
        a(einst);
    }
}

control none();
package top(none n);

top(c()) main;

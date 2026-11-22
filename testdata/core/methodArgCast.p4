/*
 * Copyright 2016 VMware, Inc.
 * SPDX-FileCopyrightText: 2016 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
extern E {
    E();
    void setValue(in bit<32> arg);
}

control c() {
    E() e;
    apply {
        e.setValue(10);
    }
}

control proto();
package top(proto p);

top(c()) main;

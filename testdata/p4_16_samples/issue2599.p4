/*
 * SPDX-FileCopyrightText: 2020 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void __semaphore_write(in bit<32> data = 0);

control c() {
    apply {
        __semaphore_write();
        __semaphore_write();
    }
}


control proto();
package top(proto p);

top(c()) main;

/*
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void func<T>(in T x);

control proto();
package top(proto _p);

control c() {
    apply {
        func(5);
    }
}

top(c()) main;

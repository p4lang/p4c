/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void f(in string s);

control c() {
    apply {
        f("boo");
    }
}

control e();
package top(e _e);

top(c()) main;

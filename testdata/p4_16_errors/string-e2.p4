/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void f<T>(in T data);

control c() {
    apply {
        f("hi");
    }
}

control e();
package top(e _e);

top(c()) main;

/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct S {
    string s;
}

control c() {
    apply {
        string v;
        v = "hi";
    }
}

control e();
package top(e _e);

top(c()) main;

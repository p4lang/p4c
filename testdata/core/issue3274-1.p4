/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

void x() {}

control c() {
    apply {
        x();
    }
}

control _c();
package top(_c _c);

top(c()) main;

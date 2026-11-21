/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

package myp(bit a);
void func() {
    myp(1w1) a;
}

control c() {
    apply {
        func();
    }
}

control C();
package top(C _c);

top(c()) main;

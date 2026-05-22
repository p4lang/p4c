/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

header H {
    bit<32> isValid;
    bit<32> setValid;
}

control c(inout H h) {
    apply {
        if (h.isValid()) {
            h.setValid = h.isValid + 32;
        }
    }
}

control C(inout H h);
package top(C c);

top(c()) main;

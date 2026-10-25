/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>

parser p();
package s(p myp);

parser myp() {
    bit<8> x;
    state start {
        transition select(8w0) {
            x &&& x: accept;
            default: reject;
        }
    }
}

s(myp()) main;
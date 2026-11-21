/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

parser MyParser1(bit tt1) {
    state start {
        transition select(1) {
                true: state1;
                _: accept;
        }
    }
    state state1 {
        transition accept;
    }
}

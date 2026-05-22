/*
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// User Program

control X();

control A() {
    apply {
    }
}
control B()(X x) {
    apply {}
}
control C()(A a) {
    apply {}
}

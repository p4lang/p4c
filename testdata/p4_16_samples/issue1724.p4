/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

action test(inout bit<16> a, inout bit<2> b) {
    b = (a << 3)[1:0];
}

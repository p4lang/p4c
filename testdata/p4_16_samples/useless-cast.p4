/*
 * SPDX-FileCopyrightText: 2017 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control c(out bit<32> y) {
    bit<32> x = 10;

    apply {
        y = (bit<32>)x;
    }
}
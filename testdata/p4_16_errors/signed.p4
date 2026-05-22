/*
 * SPDX-FileCopyrightText: 2019 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control c() {
    apply {
        int<8> signed_int1;
        int<8> signed_int2;
        signed_int1[7:0] = signed_int2;
    }
}

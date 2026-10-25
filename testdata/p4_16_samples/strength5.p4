/*
 * SPDX-FileCopyrightText: 2020 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

action a(inout bit<32> x) {
    x = x >> 3 >> 8;
}

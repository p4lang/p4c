/*
 * SPDX-FileCopyrightText: 2018 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void random<T>(out T result, in T lo);

control cIngress()
{
    bit<8> rand_val;
    apply {
        random(rand_val, 0);
    }
}

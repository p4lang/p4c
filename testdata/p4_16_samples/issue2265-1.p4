/*
 * SPDX-FileCopyrightText: 2020 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern void f<T>(in tuple<T> x);

control c() {
    apply {
        f<bit<32>>({ 32w2 });
    }
}
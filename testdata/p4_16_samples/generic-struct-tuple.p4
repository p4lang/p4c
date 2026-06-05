/*
 * SPDX-FileCopyrightText: 2021 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

struct S<T> {
    tuple<T, T> t;
}

const S<bit<32>> x = { t = { 0, 0 } };
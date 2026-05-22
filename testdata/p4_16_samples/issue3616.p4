/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

enum bit<4> e {
    a = 0,
    b = 0,
    c = (bit<4>)a,
    d = a
}
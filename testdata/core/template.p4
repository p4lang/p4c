/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern bit<32> f<T>(in T data);

action a() {
    bit<8> x;
    bit<4> y;
    bit<32> r = f(x) + f(y);
}

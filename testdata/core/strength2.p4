/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

control strength() {
    apply {
        bit<4> x;
        bit<4> y;
        bit<4> z;
        z = ~x ^ ~y;
        if (!(x < y)) {
            z = ~(x ^ (~y & ~z));
        }
        if (!(x > y)) {
            z = ~(x ^ (~y | ~z));
        }
    }
}

/*
 * SPDX-FileCopyrightText: 2022 VMware, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern E {
    E();
    bit<8> minSizeInBits<T>(T data);
    bit<8> minSizeInBits();
}

control c(out bit<8> result) {
    E() e;
    apply {
        result = e.minSizeInBits<bit<32>>(1);
        result = result + e.minSizeInBits();
    }
}

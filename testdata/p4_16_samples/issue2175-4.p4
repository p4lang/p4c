/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// In arch file
struct data {
    bit<16>     a;
};
extern Virtual {
    Virtual();
    // abstract methods must be implemented
    // by the users
    abstract bit<16> f(in bit<16> ix);
    abstract void g(inout data ix);
}

// User code
control c(inout bit<16> p) {
    Virtual() cntr = {  // implementation
        bit<16> f(in bit<16> ix) {  // abstract method implementation
            return (ix + 1);
        }
        void g(inout data x) {
        }
    };

    apply {
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);

top(c()) main;

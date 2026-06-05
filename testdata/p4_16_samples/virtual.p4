/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

// In arch file
struct data {
    bit<16>     a;
    bit<16>     b;
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
            data ix = x;
            if (ix.a < ix.b) {
                x.a = ix.a + 1;
            }
            if (ix.a > ix.b) {
                x.a = ix.a - 1;
            }
        }
    };

    apply {
        // abstract methods are not necessarily called by
        // user code (as here); they may be called by the extern
        // internally as part of its computation.
        p = cntr.f(6);
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);

top(c()) main;

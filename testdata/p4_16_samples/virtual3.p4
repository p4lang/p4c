/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>

// In arch file
extern Virtual {
    Virtual();
    // abstract methods must be implemented
    // by the users
    void increment();
    @synchronous(increment) abstract bit<16> f(in bit<16> ix);
    bit<16> total();
}

// User code
control c(inout bit<16> p) {
    bit<16>     local;

    Virtual() cntr = {  // implementation
        bit<16> f(in bit<16> ix) {  // abstract method implementation
            return (ix + local);
        }
    };
    action final_ctr() { p = cntr.total(); }
    action add_ctr() { cntr.increment(); }
    table run_ctr {
        key = { p : exact; }
        actions = { add_ctr; final_ctr; }
    }

    apply {
        // abstract methods may be called by the extern
        // internally as part of its computation.
        local = 4;
        run_ctr.apply();
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);

top(c()) main;

/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

extern Virtual {
    Virtual();
    void run(in bit<16> ix);  // internally calls f
    @synchronous(run) abstract bit<16> f(in bit<16> ix);
}

extern State {
    State(int<16> size);
    bit<16> get(in bit<16> index);
}

control c(inout bit<16> p) {
    Virtual() cntr = {
        State(1024) state;

        bit<16> f(in bit<16> ix) {  // abstract method implementation
            return state.get(ix);
        }
    };

    apply {
        cntr.run(6);
    }
}

control ctr(inout bit<16> x);
package top(ctr ctrl);

top(c()) main;

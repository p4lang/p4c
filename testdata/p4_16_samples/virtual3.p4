/*
Copyright 2013-present Barefoot Networks, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
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

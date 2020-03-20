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

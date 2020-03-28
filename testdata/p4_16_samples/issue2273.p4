/*
Copyright 2018-present Barefoot Networks, Inc.

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

extern Stack<T> {
    Stack(int size);
}

extern StackAction<T, U> {
    StackAction(Stack<T> stack);

    U pop();

    @synchronous(pop)
    @optional abstract void underflow(inout T value, out U rv);
}

header data_t {
    bit<16> h1;
}

struct headers {
    data_t data;
}

control ingress(inout headers hdr) {
    Stack<bit<16>>(2048) stack;
    StackAction<bit<16>, bit<16>>(stack) read = {
        void underflow(inout bit<16> value, out bit<16> rv) {
            rv = 0xf0f0;
        }
    };

    apply {
        hdr.data.h1 = read.pop();
    }
}

control ctr<H>(inout H hdr);
package top<H>(ctr<H> ctrl);

top(ingress()) main;

/*
Copyright 2017 Xilinx, Inc.

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

header header_h {
    bit<8> field;
}

struct struct_t {
    header_h[4]     stack;
}

control ctrl(inout struct_t input, out bit<8> output) {
    bit<8> tmp0;
    bit<8> tmp1;
    action act() {
        tmp0 = input.stack[0].field;
        input.stack.pop_front(1);
        tmp1 = tmp0;
    }
    apply {
        act();
        output = tmp1;
    }
}

control MyControl<S,H>(inout S data, out H output);
package MyPackage<S,H>(MyControl<S,H> ctrl);
MyPackage(ctrl()) main;

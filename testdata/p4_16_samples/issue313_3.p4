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
    header_h hdr;
}

control ctrl(inout struct_t input, out bit<8> out1, out header_h out2) {
    bit<8> tmp0;
    bit<8> tmp1;
    header_h tmp2;
    header_h tmp3;
    action act() {
        tmp0 = input.hdr.field;
        input.hdr.setValid();
        tmp1 = tmp0;

        tmp2 = input.hdr;
        input.hdr.setInvalid();
        tmp3 = tmp2;
    }
    apply {
        act();
        out1 = tmp1;
        out2 = tmp3;
    }
}

control MyControl<S,H>(inout S i, out bit<8> o1, out H o2);
package MyPackage<S,H>(MyControl<S,H> c);
MyPackage(ctrl()) main;

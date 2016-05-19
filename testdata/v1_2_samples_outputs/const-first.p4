# Copyright 2013-present Barefoot Networks, Inc. 
# 
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
# 
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

const bit<32> zero = 32w0;
const bit<48> tooLarge = 48w0xbbccddeeff00;
const bit<32> one = 32w1;
const bit<32> max = 32w0xffffffff;
const bit<32> z = 32w1;
struct S {
    bit<32> a;
    bit<32> b;
}

const S v = { 32w3, 32w1 };
const bit<32> two = 32w2;
const int<32> twotwo = 32s2;
const bit<32> twothree = 32w2;
const bit<6> twofour = 6w2;
struct T {
    S a;
    S b;
}

const T zz = { { 32w0, 32w1 }, { 32w2, 32w3 } };
const bit<32> x = 32w0;
const bit<32> x1 = 32w4294967295;
typedef int<32> int32;
const int32 izero = 32s0;

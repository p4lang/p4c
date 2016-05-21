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

const bit<32> b0 = 32w0xFF;       // a 32-bit bit-string with value 00FF
const int<32> b2 = 32s0xFF;       // a 32-bit signed number with value 255
const int<32> b3 = -32s0xFF;      // a 32-bit signed number with value -255
const bit<8> b4 = 8w0b10101010;   // an 8-bit bit-string with value AA
const bit<8> b5 = 8w0b_1010_1010; // same value as above
const bit<8> b6 = 8w170;          // same value as above
const bit<8> b7 = 8w0b1010_1010;  // an 8-bit unsigned number with value 170
const int<8> b8 = 8s0b1010_1010;  // an 8-bit signed number with value -86

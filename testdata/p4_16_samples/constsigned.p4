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

const int<8> a = 0;
const int<8> b = -1;
const int<8> c = -2;
const int<8> d = -127;
const int<8> e = -128;
const int<8> f = -129;
const int<8> g = -255;
const int<8> h = -256;
const int<8> i = 1;
const int<8> j = 2;
const int<8> k = 127;
const int<8> l = 128;
const int<8> m = 129;
const int<8> n = 255;
const int<8> o = 256;

const int<8> a0 = 8s0;
const int<8> b0 = -8s1;
const int<8> c0 = -8s2;
const int<8> d0 = -8s127;
const int<8> e0 = -8s128;
const int<8> f0 = -8s129;
const int<8> g0 = -8s255;
const int<8> h0 = -8s256;
const int<8> i0 = 8s1;
const int<8> j0 = 8s2;
const int<8> k0 = 8s127;
const int<8> l0 = 8s128;
const int<8> m0 = 8s129;
const int<8> n0 = 8s255;
const int<8> o0 = 8s256;

const int<1> zz0 = 0;
const int<1> zz1 = 1;
const int<2> zz2 = 2;
const int<1> zz3 = (int<1>) zz2[0:0];

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

control p()
{
    apply {
        int<32> a = 32s1;
        int<32> b = 32s1;
        int<32> c;
        bit<32> f;
        bit<16> e;
        bool    d;
    
        c = +b;
        c = -b;
        f = ~(bit<32>)b;
        f = (bit<32>)a & (bit<32>)b;
        f = (bit<32>)a | (bit<32>)b;
        f = (bit<32>)a ^ (bit<32>)b;
        f = (bit<32>)a << (bit<32>)b;
        f = (bit<32>)a >> (bit<32>)b;
        f = (bit<32>)a >> 4;
        f = (bit<32>)a << 6;
        c = a * b;
        e = ((bit<32>)a)[15:0];
        f = e ++ e;
        c = d ? a : b;
    
        d = a == b;
        d = a != b;
        d = a < b;
        d = a > b;
        d = a <= b;
        d = a >= b;
    
        d = !d;
        d = d && d;
        d = d || d;
        d = d == d;
        d = d != d;
    }
}

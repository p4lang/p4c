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

control strength() {
    apply {
        bit<4> x;
        x = x & 4w0;
        x = 4w0 & x;
        x = x | 4w0;    
        x = 4w0 | x;
        x = 4w0 ^ x;
        x = x ^ 4w0;
        x = x << 4w0;
        x = x >> 4w0;
        bit<4> y;
        y = y + 4w0;
        y = 4w0 + y;
        y = y - 4w0;
        y = 4w0 - y;
        y = y * 4w0;
        y = y * 4w1;
        y = 4w1 * y;
        y = 4w0 * y;
        y = y * 2;
        y = y * 16;
        y = 8 * y;
        y = y / 2;
        y = y / 8;
        y = y / 4w1;
        y = 4w0 / y;
        y = y % 4w1;
        y = y % 4;
        y = y - 4w1;    
        y = y - 4w0xF;
        int<4> w;
        w = w - 4s1;
        w = w - 4s0xF;
        bool z;
        z = z && true;
        z = true && z;
        z = z && false;
        z = false && z;
        z = z || true;
        z = true || z;
        z = z || false;
        z = false || z;            
    }
}

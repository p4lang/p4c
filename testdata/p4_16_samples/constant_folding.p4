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

control proto(out bit<32> x);
package top(proto _c);

control c(out bit<32> x) {
    apply {
        x = 5 + 3;
        x = 32w5 + 3;
        x = 32w5 + 32w3;
        x = 5 + 32w3;

        x = 5 - 3;
        x = 32w5 - 3;
        x = 32w5 - 32w3;
        x = 5 - 32w3;

        x = 5 * 3;
        x = 32w5 * 3;

        x = 5 / 3;
        x = 32w5 / 3;

        x = 5 % 3;
        x = 32w5 / 3;

        x = 5 & 3;
        x = 32w5 & 3;

        x = 5 | 3;
        x = 32w5 | 3;

        x = 5 ^ 3;
        x = 32w5 ^ 3;

        x = 5 << 3;
        x = 32w5 << 3;

        x = 5 >> 1;
        x = 32w5 >> 1;

        x = 5 << 0;
        x = 5 >> 0;

        x = (bit<32>)(4w1 ++ 4w1);

        bool w;
        w = 5 == 3;
        w = 32w5 == 3;

        w = 5 != 3;
        w = 32w5 != 3;

        w = 5 < 3;
        w = 32w5 < 3;

        w = 5 > 3;
        w = 32w5 > 3;

        w = 5 <= 3;
        w = 32w5 <= 3;

        w = 5 >= 3;
        w = 32w5 >= 3;

        w = true == false;
        w = true != false;

        w = true == true;
        w = true != true;

        // overflows
        bit<8> z;
        z = 128 + 128;
        z = 8w128 + 128;

        z = 0 - 128;
        z = 8w0 - 128;

        z = 1 << 9;
        z = 8w1 << 9;
        z = 10 >> 9;
    }
}

top(c()) main;

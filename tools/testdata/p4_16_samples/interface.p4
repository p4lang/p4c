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

#include <core.p4>

extern Crc16 <T> {
        Crc16();
        Crc16(int<32> x);
        void initialize<U>(in U input_data);
        T value();
        T make_index(
            in T size,
            in T base
        );
}

control p() {
    Crc16<bit<32>>() crc0;
    Crc16<int<32>>(32s0) crc1;
    Crc16<int<32>>()  crc2;

    apply {}
}

control empty();

package m(empty e);

m(p()) main;

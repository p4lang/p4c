/*
Copyright 2018 Barefoot Networks, Inc.

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

extern hash_function<O, T> {
    // FIXME -- needs to be a way to do this with T a type param of hash
    // instead of hash_function
    O hash(in T data);
}

extern hash_function<O, T> crc_poly<O, T>(O poly);

header h1_t {
    bit<32>     f1;
    bit<32>     f2;
    bit<32>     f3;
}

struct hdrs {
    h1_t        h1;
    bit<16>     crc;
}

control test(inout hdrs hdr) {
    apply {
        hdr.crc = crc_poly<bit<16>, h1_t>(16w0x801a).hash(hdr.h1);
    }
}

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

struct Version {
    bit<8> major;
    bit<8> minor;
}

error {
    NoError,
    PacketTooShort,
    NoMatch,
    EmptyStack,
    FullStack,
    OverwritingHeader
}

extern packet_in {
    void extract<T>(out T hdr);
    void extract<T>(out T variableSizeHeader, in bit<32> sizeInBits);
    T lookahead<T>();
    void advance(in bit<32> sizeInBits);
    bit<32> length();
}

extern packet_out {
    void emit<T>(in T hdr);
}

match_kind {
    exact,
    ternary,
    lpm
}

header Hdr {
    bit<8>      size;
    @length((bit<32>)size) 
    varbit<256> data0;
    varbit<12>  data1;
    varbit<256> data2;
    bit<32>     size2;
    @length(size2 + (bit<32>)(size << 4)) 
    varbit<128> data3;
}

header Hdr_h {
    bit<4>     size;
    @length((bit<32>)(size + 4w8)) 
    varbit<16> data0;
}

struct Ph_t {
    Hdr_h h;
}

header Hdr_top_h {
    bit<4> size;
}

header Hdr_bot_h {
    varbit<16> data0;
}

struct Ph1_t {
    Hdr_top_h htop;
    Hdr_bot_h h;
}


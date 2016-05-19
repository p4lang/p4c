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

const Version P4_VERSION = { 8w1, 8w2 };
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

extern void verify(in bool check, in error toSignal);
action NoAction() {
}
match_kind {
    exact,
    ternary,
    lpm
}

header Ipv4_no_options_h {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header Ipv4_options_h {
    varbit<160> options;
}

struct Tcp {
    bit<16> port;
}

struct Parsed_headers {
    Ipv4_no_options_h ipv4;
    Ipv4_options_h    ipv4options;
    Tcp               tcp;
}

error {
    InvalidOptions
}

parser Top(packet_in b, out Parsed_headers headers) {
    state start {
        transition parse_ipv4;
    }
    state parse_ipv4 {
        b.extract(headers.ipv4);
        verify(headers.ipv4.ihl >= 4w5, InvalidOptions);
        transition parse_ipv4_options;
    }
    state parse_ipv4_options {
        b.extract(headers.ipv4options, (bit<32>)(((bit<8>)headers.ipv4.ihl << 2) + 8w236 << 3));
        transition select(headers.ipv4.protocol) {
            8w6: parse_tcp;
            8w17: parse_udp;
            default: accept;
        }
    }
    state parse_tcp {
        b.extract(headers.tcp);
        transition select(headers.tcp.port) {
            16w0 &&& 16w0xfc00: well_known_port;
            default: other_port;
        }
    }
    state well_known_port {
    }
    state other_port {
    }
    state parse_udp {
    }
}


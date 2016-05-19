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

header Mpls_h {
    bit<20> label;
    bit<3>  tc;
    bit<1>  bos;
    bit<8>  ttl;
}

control p() {
    apply {
        Mpls_h[10] mpls_vec;
    }
}

header Tcp_option_end_h {
    bit<8> kind;
}

header Tcp_option_nop_h {
    bit<8> kind;
}

header Tcp_option_ss_h {
    bit<8>  kind;
    bit<32> maxSegmentSize;
}

header Tcp_option_s_h {
    bit<8>  kind;
    bit<24> scale;
}

header Tcp_option_sack_h {
    bit<8>      kind;
    bit<8>      length;
    varbit<256> sack;
}

header_union Tcp_option_h {
    Tcp_option_end_h  end;
    Tcp_option_nop_h  nop;
    Tcp_option_ss_h   ss;
    Tcp_option_s_h    s;
    Tcp_option_sack_h sack;
}

typedef Tcp_option_h[10] Tcp_option_stack;
struct Tcp_option_sack_top {
    bit<8> kind;
    bit<8> length;
}

parser Tcp_option_parser(packet_in b, out Tcp_option_stack vec) {
    state start {
        transition select((b.lookahead<bit<8>>())) {
            8w0x0: parse_tcp_option_end;
            8w0x1: parse_tcp_option_nop;
            8w0x2: parse_tcp_option_ss;
            8w0x3: parse_tcp_option_s;
            8w0x5: parse_tcp_option_sack;
        }
    }
    state parse_tcp_option_end {
        b.extract(vec.next.end);
        transition accept;
    }
    state parse_tcp_option_nop {
        b.extract(vec.next.nop);
        transition start;
    }
    state parse_tcp_option_ss {
        b.extract(vec.next.ss);
        transition start;
    }
    state parse_tcp_option_s {
        b.extract(vec.next.s);
        transition start;
    }
    state parse_tcp_option_sack {
        b.extract(vec.next.sack, (bit<32>)(b.lookahead<Tcp_option_sack_top>()).length);
        transition start;
    }
}


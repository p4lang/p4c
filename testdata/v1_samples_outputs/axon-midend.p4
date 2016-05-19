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

match_kind {
    range,
    selector
}

struct standard_metadata_t {
    bit<9>  ingress_port;
    bit<9>  egress_spec;
    bit<9>  egress_port;
    bit<32> clone_spec;
    bit<32> instance_type;
    bit<1>  drop;
    bit<16> recirculate_port;
    bit<32> packet_length;
}

extern Checksum16 {
    bit<16> get<D>(in D data);
}

enum CounterType {
    Packets,
    Bytes,
    Both
}

extern Counter {
    Counter(bit<32> size, CounterType type);
    void increment(in bit<32> index);
}

extern DirectCounter {
    DirectCounter(CounterType type);
}

extern Meter {
    Meter(bit<32> size, CounterType type);
    void meter<T>(in bit<32> index, out T result);
}

extern DirectMeter<T> {
    DirectMeter(CounterType type);
    void read(out T result);
}

extern Register<T> {
    Register(bit<32> size);
    void read(out T result, in bit<32> index);
    void write(in bit<32> index, in T value);
}

extern ActionProfile {
    ActionProfile(bit<32> size);
}

enum HashAlgorithm {
    crc32,
    crc16,
    random,
    identity
}

extern void mark_to_drop();
extern ActionSelector {
    ActionSelector(HashAlgorithm algorithm, bit<32> size, bit<32> outputWidth);
}

parser Parser<H, M>(packet_in b, out H parsedHdr, inout M meta, inout standard_metadata_t standard_metadata);
control VerifyChecksum<H, M>(in H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control Ingress<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control Egress<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control ComputeCkecksum<H, M>(inout H hdr, inout M meta, inout standard_metadata_t standard_metadata);
control Deparser<H>(packet_out b, in H hdr);
package V1Switch<H, M>(Parser<H, M> p, VerifyChecksum<H, M> vr, Ingress<H, M> ig, Egress<H, M> eg, ComputeCkecksum<H, M> ck, Deparser<H> dep);
struct my_metadata_t {
    bit<8>  fwdHopCount;
    bit<8>  revHopCount;
    bit<16> headerLen;
}

header axon_head_t {
    bit<64> preamble;
    bit<8>  axonType;
    bit<16> axonLength;
    bit<8>  fwdHopCount;
    bit<8>  revHopCount;
}

header axon_hop_t {
    bit<8> port;
}

struct metadata {
    @name("my_metadata") 
    my_metadata_t my_metadata;
}

struct headers {
    @name("axon_head") 
    axon_head_t    axon_head;
    @name("axon_fwdHop") 
    axon_hop_t[64] axon_fwdHop;
    @name("axon_revHop") 
    axon_hop_t[64] axon_revHop;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("parse_fwdHop") state parse_fwdHop {
        packet.extract(hdr.axon_fwdHop.next);
        meta.my_metadata.fwdHopCount = meta.my_metadata.fwdHopCount + 8w255;
        transition parse_next_fwdHop;
    }
    @name("parse_head") state parse_head {
        packet.extract(hdr.axon_head);
        meta.my_metadata.fwdHopCount = hdr.axon_head.fwdHopCount;
        meta.my_metadata.revHopCount = hdr.axon_head.revHopCount;
        meta.my_metadata.headerLen = (bit<16>)(8w2 + hdr.axon_head.fwdHopCount + hdr.axon_head.revHopCount);
        transition select(hdr.axon_head.fwdHopCount) {
            8w0: accept;
            default: parse_next_fwdHop;
        }
    }
    @name("parse_next_fwdHop") state parse_next_fwdHop {
        transition select(meta.my_metadata.fwdHopCount) {
            8w0x0: parse_next_revHop;
            default: parse_fwdHop;
        }
    }
    @name("parse_next_revHop") state parse_next_revHop {
        transition select(meta.my_metadata.revHopCount) {
            8w0x0: accept;
            default: parse_revHop;
        }
    }
    @name("parse_revHop") state parse_revHop {
        packet.extract(hdr.axon_revHop.next);
        meta.my_metadata.revHopCount = meta.my_metadata.revHopCount + 8w255;
        transition parse_next_revHop;
    }
    @name("start") state start {
        transition select(packet.lookahead<bit<64>>()[63:0]) {
            64w0: parse_head;
            default: accept;
        }
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    action NoAction_1() {
    }
    action NoAction_2() {
    }
    @name("_drop") action _drop() {
        mark_to_drop();
    }
    @name("_drop") action _drop_1() {
        mark_to_drop();
    }
    @name("route") action route() {
        standard_metadata.egress_spec = (bit<9>)hdr.axon_fwdHop[0].port;
        hdr.axon_head.fwdHopCount = hdr.axon_head.fwdHopCount + 8w255;
        hdr.axon_fwdHop.pop_front(1);
        hdr.axon_head.revHopCount = hdr.axon_head.revHopCount + 8w1;
        hdr.axon_revHop.push_front(1);
        hdr.axon_revHop[0].port = (bit<8>)standard_metadata.ingress_port;
    }
    @name("drop_pkt") table drop_pkt_0() {
        actions = {
            _drop;
            NoAction_1;
        }
        size = 1;
        default_action = NoAction_1();
    }
    @name("route_pkt") table route_pkt_0() {
        actions = {
            _drop_1;
            route;
            NoAction_2;
        }
        key = {
            hdr.axon_head.isValid()     : exact;
            hdr.axon_fwdHop[0].isValid(): exact;
        }
        size = 1;
        default_action = NoAction_2();
    }
    apply {
        if (hdr.axon_head.axonLength != meta.my_metadata.headerLen) 
            drop_pkt_0.apply();
        else 
            route_pkt_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.axon_head);
        packet.emit(hdr.axon_fwdHop);
        packet.emit(hdr.axon_revHop);
    }
}

control verifyChecksum(in headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

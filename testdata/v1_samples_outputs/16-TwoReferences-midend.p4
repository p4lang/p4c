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
header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> ethertype;
}

struct metadata {
}

struct headers {
    @name("ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name("start") state start {
        packet.extract(hdr.ethernet);
        transition accept;
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
    action NoAction_3() {
    }
    action NoAction_4() {
    }
    action NoAction_5() {
    }
    action NoAction_6() {
    }
    @name("do_b") action do_b() {
    }
    @name("do_d") action do_d() {
    }
    @name("do_e") action do_e() {
    }
    @name("nop") action nop() {
    }
    @name("nop") action nop_1() {
    }
    @name("nop") action nop_2() {
    }
    @name("nop") action nop_3() {
    }
    @name("nop") action nop_4() {
    }
    @name("A") table A_0() {
        actions = {
            do_b;
            do_d;
            do_e;
            NoAction_1;
        }
        key = {
            hdr.ethernet.dstAddr: exact;
        }
        default_action = NoAction_1();
    }
    @name("B") table B_0() {
        actions = {
            nop;
            NoAction_2;
        }
        default_action = NoAction_2();
    }
    @name("C") table C_0() {
        actions = {
            nop_1;
            NoAction_3;
        }
        default_action = NoAction_3();
    }
    @name("D") table D_0() {
        actions = {
            nop_2;
            NoAction_4;
        }
        default_action = NoAction_4();
    }
    @name("E") table E_0() {
        actions = {
            nop_3;
            NoAction_5;
        }
        default_action = NoAction_5();
    }
    @name("F") table F_0() {
        actions = {
            nop_4;
            NoAction_6;
        }
        default_action = NoAction_6();
    }
    apply {
        switch (A_0.apply().action_run) {
            do_b: {
                B_0.apply();
                C_0.apply();
            }
            do_d: {
                D_0.apply();
                C_0.apply();
            }
            do_e: {
                E_0.apply();
            }
        }

        F_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
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

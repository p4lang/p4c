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

extern void verify(in bool check, in error toSignal);
match_kind {
    exact,
    ternary,
    lpm
}

extern Checksum16 {
    void clear();
    void update<D>(in D dt);
    void update<D>(in bool condition, in D dt);
    bit<16> get();
}

typedef bit<4> PortId_t;
struct InControl {
    PortId_t inputPort;
}

struct OutControl {
    PortId_t outputPort;
}

parser Parser<H>(packet_in b, out H parsedHeaders);
control MAP<H>(inout H headers, in error parseError, in InControl inCtrl, out OutControl outCtrl);
control Deparser<H>(inout H outputHeaders, packet_out b);
package Simple<H>(Parser<H> p, MAP<H> map, Deparser<H> d);
typedef bit<48> @ethernetaddress EthernetAddress;
typedef bit<32> @ipv4address IPv4Address;
header Ethernet_h {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header IPv4_h {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     totalLen;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     fragOffset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdrChecksum;
    IPv4Address srcAddr;
    IPv4Address dstAddr;
}

error {
    IPv4IncorrectVersion,
    IPv4OptionsNotSupported,
    IPv4ChecksumError
}

struct Parsed_packet {
    Ethernet_h ethernet;
    IPv4_h     ip;
}

parser TopParser(packet_in b, out Parsed_packet p) {
    Checksum16() @name("ck") ck_0;
    state start {
        b.extract(p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x800: parse_ipv4;
        }
    }
    state parse_ipv4 {
        b.extract(p.ip);
        verify(p.ip.version == 4w4, IPv4IncorrectVersion);
        verify(p.ip.ihl == 4w5, IPv4OptionsNotSupported);
        ck_0.clear();
        ck_0.update(p.ip);
        verify(ck_0.get() == 16w0, IPv4ChecksumError);
        transition accept;
    }
}

control Pipe(inout Parsed_packet headers, in error parseError, in InControl inCtrl, out OutControl outCtrl) {
    IPv4Address nextHop_0;
    IPv4Address nextHop_2;
    IPv4Address nextHop_3;
    bool hasReturned;
    action NoAction_1() {
    }
    @name("Drop_action") action Drop_action() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("Drop_action") action Drop_action_1() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("Drop_action") action Drop_action_2() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("Drop_action") action Drop_action_3() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("Set_nhop") action Set_nhop(out IPv4Address nextHop_1, IPv4Address ipv4_dest, PortId_t port) {
        nextHop_1 = ipv4_dest;
        headers.ip.ttl = headers.ip.ttl + 8w255;
        outCtrl.outputPort = port;
    }
    @name("ipv4_match") table ipv4_match_0() {
        key = {
            headers.ip.dstAddr: lpm;
        }
        actions = {
            Set_nhop(nextHop_2);
            Drop_action;
        }
        size = 1024;
        default_action = Drop_action;
    }
    @name("Send_to_cpu") action Send_to_cpu() {
        outCtrl.outputPort = 4w0xe;
    }
    @name("check_ttl") table check_ttl_0() {
        key = {
            headers.ip.ttl: exact;
        }
        actions = {
            Send_to_cpu;
            NoAction_1;
        }
        const default_action = NoAction_1;
    }
    @name("Set_dmac") action Set_dmac(EthernetAddress dmac) {
        headers.ethernet.dstAddr = dmac;
    }
    @name("dmac") table dmac_0() {
        key = {
            nextHop_3: exact;
        }
        actions = {
            Set_dmac;
            Drop_action_1;
        }
        size = 1024;
        default_action = Drop_action_1;
    }
    @name("Rewrite_smac") action Rewrite_smac(EthernetAddress sourceMac) {
        headers.ethernet.srcAddr = sourceMac;
    }
    @name("smac") table smac_0() {
        key = {
            outCtrl.outputPort: exact;
        }
        actions = {
            Drop_action_2;
            Rewrite_smac;
        }
        size = 16;
        default_action = Drop_action_2;
    }
    action act() {
        hasReturned = true;
    }
    action act_0() {
        hasReturned = false;
    }
    action act_1() {
        hasReturned = true;
    }
    action act_2() {
        nextHop_0 = nextHop_2;
    }
    action act_3() {
        hasReturned = true;
    }
    action act_4() {
        nextHop_3 = nextHop_0;
    }
    action act_5() {
        hasReturned = true;
    }
    table tbl_act() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    table tbl_Drop_action() {
        actions = {
            Drop_action_3();
        }
        const default_action = Drop_action_3();
    }
    table tbl_act_0() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_1() {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    table tbl_act_2() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_3() {
        actions = {
            act_3();
        }
        const default_action = act_3();
    }
    table tbl_act_4() {
        actions = {
            act_4();
        }
        const default_action = act_4();
    }
    table tbl_act_5() {
        actions = {
            act_5();
        }
        const default_action = act_5();
    }
    apply {
        tbl_act.apply();
        if (parseError != NoError) {
            tbl_Drop_action.apply();
            tbl_act_0.apply();
        }
        if (!hasReturned) {
            ipv4_match_0.apply();
            tbl_act_1.apply();
            if (outCtrl.outputPort == 4w0xf) 
                tbl_act_2.apply();
        }
        if (!hasReturned) {
            check_ttl_0.apply();
            if (outCtrl.outputPort == 4w0xe) 
                tbl_act_3.apply();
        }
        if (!hasReturned) {
            tbl_act_4.apply();
            dmac_0.apply();
            if (outCtrl.outputPort == 4w0xf) 
                tbl_act_5.apply();
        }
        if (!hasReturned) 
            smac_0.apply();
    }
}

control TopDeparser(inout Parsed_packet p, packet_out b) {
    Checksum16() @name("ck") ck_1;
    action act_6() {
        ck_1.clear();
        p.ip.hdrChecksum = 16w0;
        ck_1.update(p.ip);
        p.ip.hdrChecksum = ck_1.get();
    }
    table tbl_act_6() {
        actions = {
            act_6();
        }
        const default_action = act_6();
    }
    apply {
        b.emit(p.ethernet);
        if (p.ip.isValid()) {
            tbl_act_6.apply();
            b.emit(p.ip);
        }
    }
}

Simple(TopParser(), Pipe(), TopDeparser()) main;

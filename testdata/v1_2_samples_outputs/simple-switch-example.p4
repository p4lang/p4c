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

#include "/home/mbudiu/git/p4c/build/../p4include/core.p4"
#include "../testdata/v1_2_samples/simple_model.p4"

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
    Checksum16() ck;
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
        ck.clear();
        ck.update(p.ip);
        verify(ck.get() == 16w0, IPv4ChecksumError);
        transition accept;
    }
}

control Pipe(inout Parsed_packet headers, in error parseError, in InControl inCtrl, out OutControl outCtrl) {
    action Drop_action() {
        outCtrl.outputPort = DROP_PORT;
    }
    action Set_nhop(out IPv4Address nextHop, IPv4Address ipv4_dest, PortId_t port) {
        nextHop = ipv4_dest;
        headers.ip.ttl = headers.ip.ttl - 8w1;
        outCtrl.outputPort = port;
    }
    table ipv4_match(out IPv4Address nextHop) {
        key = {
            headers.ip.dstAddr: lpm;
        }
        actions = {
            Set_nhop(nextHop);
            Drop_action;
        }
        size = 1024;
        default_action = Drop_action;
    }
    action Send_to_cpu() {
        outCtrl.outputPort = CPU_OUT_PORT;
    }
    table check_ttl() {
        key = {
            headers.ip.ttl: exact;
        }
        actions = {
            Send_to_cpu;
            NoAction;
        }
        const default_action = NoAction;
    }
    action Set_dmac(EthernetAddress dmac) {
        headers.ethernet.dstAddr = dmac;
    }
    table dmac(in IPv4Address nextHop) {
        key = {
            nextHop: exact;
        }
        actions = {
            Set_dmac;
            Drop_action;
        }
        size = 1024;
        default_action = Drop_action;
    }
    action Rewrite_smac(EthernetAddress sourceMac) {
        headers.ethernet.srcAddr = sourceMac;
    }
    table smac() {
        key = {
            outCtrl.outputPort: exact;
        }
        actions = {
            Drop_action;
            Rewrite_smac;
        }
        size = 16;
        default_action = Drop_action;
    }
    apply {
        if (parseError != NoError) {
            Drop_action();
            return;
        }
        IPv4Address nextHop;
        ipv4_match.apply(nextHop);
        if (outCtrl.outputPort == DROP_PORT) 
            return;
        check_ttl.apply();
        if (outCtrl.outputPort == CPU_OUT_PORT) 
            return;
        dmac.apply(nextHop);
        if (outCtrl.outputPort == DROP_PORT) 
            return;
        smac.apply();
    }
}

control TopDeparser(inout Parsed_packet p, packet_out b) {
    Checksum16() ck;
    apply {
        b.emit(p.ethernet);
        if (p.ip.isValid()) {
            ck.clear();
            p.ip.hdrChecksum = 16w0;
            ck.update(p.ip);
            p.ip.hdrChecksum = ck.get();
            b.emit(p.ip);
        }
    }
}

Simple(TopParser(), Pipe(), TopDeparser()) main;

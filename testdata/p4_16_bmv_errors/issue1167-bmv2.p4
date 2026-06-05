/*
 * Copyright 2017 Cisco Systems, Inc.
 * SPDX-FileCopyrightText: 2017 Cisco Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <core.p4>
#include <v1model.p4>

typedef bit<48>  EthernetAddress;

header Ethernet_h {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct Parsed_packet {
    Ethernet_h    ethernet;
}

struct mystruct1 {
    bit<4>  a;
    bit<4>  b;
}

enum MeterColor_t {GREEN, YELLOW, RED};

control DeparserI(packet_out packet,
                  in Parsed_packet hdr) {
    apply { packet.emit(hdr.ethernet); }
}

parser parserI(packet_in pkt,
               out Parsed_packet hdr,
               inout mystruct1 meta,
               inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(hdr.ethernet);
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr,
                 inout mystruct1 meta,
                 inout standard_metadata_t stdmeta) {
    direct_meter<MeterColor_t>(MeterType.bytes) my_meter;
    MeterColor_t egress_color = MeterColor_t.GREEN;
    action foo() {
        meta.b = meta.b + 5;
        // Uncommenting the following line causes "Compiler Bug" in
        // output to go away, but the BMv2 JSON file produced has no
        // "read" method call in the definition of the action "foo".
        //my_meter.read(egress_color);
    }
    table guh {
        key = { hdr.ethernet.srcAddr : exact; }
        actions = { foo; }
        default_action = foo;
        meters = my_meter;
    }

    apply {
        guh.apply();
    }
}

control cEgress(inout Parsed_packet hdr,
                inout mystruct1 meta,
                inout standard_metadata_t stdmeta) {
    apply { }
}

control vc(inout Parsed_packet hdr,
           inout mystruct1 meta) {
    apply { }
}

control uc(inout Parsed_packet hdr,
           inout mystruct1 meta) {
    apply { }
}

V1Switch<Parsed_packet, mystruct1>(parserI(),
                                   vc(),
                                   cIngress(),
                                   cEgress(),
                                   uc(),
                                   DeparserI()) main;

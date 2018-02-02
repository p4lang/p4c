#include <core.p4>
#include <v1model.p4>

typedef bit<48> EthernetAddress;
header X {
    bit<32> add1;
}

struct Parsed_packet {
    X x;
}

struct meta {
    bit<32> add2;
}

@name("metadata") struct metadata {
    meta    x;
    bit<32> z;
}

control DeparserI(packet_out packet, in Parsed_packet hdr) {
    apply {
    }
}

parser parserI(packet_in pkt, out Parsed_packet hdr, inout metadata m, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract<X>(hdr.x);
        transition accept;
    }
}

control cIngress(inout Parsed_packet hdr, inout metadata m, inout standard_metadata_t stdmeta) {
    bit<32> z = 32w5;
    action foo() {
    }
    table t {
        key = {
            hdr.x.add1: exact @name("hdr.x.add1") ;
            m.x.add2  : exact @name("m.x.add2") ;
            m.z       : exact @name("m.z") ;
            z         : exact @name("z") ;
        }
        actions = {
            foo();
        }
        default_action = foo();
    }
    apply {
        t.apply();
    }
}

control cEgress(inout Parsed_packet hdr, inout metadata m, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(inout Parsed_packet hdr, inout metadata m) {
    apply {
    }
}

control uc(inout Parsed_packet hdr, inout metadata m) {
    apply {
    }
}

V1Switch<Parsed_packet, metadata>(parserI(), vc(), cIngress(), cEgress(), uc(), DeparserI()) main;


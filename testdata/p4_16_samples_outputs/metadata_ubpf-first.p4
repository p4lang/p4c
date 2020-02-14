#include <core.p4>
#include <ubpf_model.p4>

@ethernetaddress typedef bit<48> EthernetAddress;
header Ethernet_h {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

struct Headers_t {
    Ethernet_h ethernet;
}

struct metadata {
    bit<16> etherType;
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta) {
    state start {
        p.extract<Ethernet_h>(headers.ethernet);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta) {
    action fill_metadata() {
        meta.etherType = headers.ethernet.etherType;
    }
    table tbl {
        key = {
            headers.ethernet.etherType: exact @name("headers.ethernet.etherType") ;
        }
        actions = {
            fill_metadata();
            NoAction();
        }
        const default_action = NoAction();
    }
    action change_etherType() {
        headers.ethernet.etherType = 16w0x86dd;
    }
    table meta_based_tbl {
        key = {
            meta.etherType: exact @name("meta.etherType") ;
        }
        actions = {
            change_etherType();
            NoAction();
        }
        const default_action = NoAction();
    }
    apply {
        tbl.apply();
        meta_based_tbl.apply();
    }
}

control DeparserImpl(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit<Ethernet_h>(headers.ethernet);
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), DeparserImpl()) main;


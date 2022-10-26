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

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract(headers.ethernet);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    action fill_metadata() {
        meta.etherType = headers.ethernet.etherType;
    }
    table tbl {
        key = {
            headers.ethernet.etherType: exact;
        }
        actions = {
            fill_metadata;
            NoAction;
        }
    }
    action change_etherType() {
        headers.ethernet.etherType = 0x86dd;
    }
    table meta_based_tbl {
        key = {
            meta.etherType: exact;
        }
        actions = {
            change_etherType;
            NoAction;
        }
    }
    apply {
        tbl.apply();
        meta_based_tbl.apply();
    }
}

control DeparserImpl(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit(headers.ethernet);
    }
}

ubpf(prs(), pipe(), DeparserImpl()) main;

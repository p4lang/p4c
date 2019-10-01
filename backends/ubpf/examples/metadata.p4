#include <core.p4>
#include "ubpf_filter_model.p4"

@ethernetaddress typedef bit<48> EthernetAddress;

// standard Ethernet header
header Ethernet_h
{
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16> etherType;
}

struct Headers_t
{
    Ethernet_h ethernet;
}

struct metadata {
    bit<16> etherType;
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta) {
    state start {
        p.extract(headers.ethernet);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta) {

    action fill_metadata() {
        meta.etherType = headers.ethernet.etherType;
    }

    table tbl {
        key = {
            headers.ethernet.etherType : exact;
        }
        actions = {
            fill_metadata;
            NoAction;
        }

        const default_action = NoAction;
    }

    action change_etherType() {
        // set etherType to IPv6. Just to show that metadata works.
        headers.ethernet.etherType = 0x86DD;
    }

    table meta_based_tbl {
        key = {
            meta.etherType : exact;
        }
        actions = {
            change_etherType;
            NoAction;
        }

        const default_action = NoAction;
    }

    apply {
        tbl.apply();
        meta_based_tbl.apply();
    }

}

ubpf(prs(), pipe()) main;


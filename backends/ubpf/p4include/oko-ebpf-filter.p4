#include <filter_model.p4>
#include <core.p4>

@ethernetaddress typedef bit<48> EthernetAddress;
@ipv4address     typedef bit<32>     IPv4Address;

// standard Ethernet header
header Ethernet_h
{
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16> etherType;
}

// IPv4 header without options
header IPv4_h {
    bit<4>       version;
    bit<4>       ihl;
    bit<8>       diffserv;
    bit<16>      totalLen;
    bit<16>      identification;
    bit<3>       flags;
    bit<13>      fragOffset;
    bit<8>       ttl;
    bit<8>       protocol;
    bit<16>      hdrChecksum;
    IPv4Address  srcAddr;
    IPv4Address  dstAddr;
}

struct Headers_t
{
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800 : ip;
            default : reject;
        }
    }

    state ip {
        p.extract(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers) {
    action Reject() {
        mark_to_drop();
    }

    table filter_tbl {
        key = { }
        actions = {
            Reject;
            NoAction;
        }

        implementation = hash_table(1024);
        const default_action = NoAction;
    }

    apply {
        filter_tbl.apply();
    }
}

Filter(prs(), pipe()) main;

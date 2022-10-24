#include <core.p4>
#include <ubpf_model.p4>

@ethernetaddress typedef bit<48> EthernetAddress;
@ipv4address typedef bit<32> IPv4Address;
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

struct Headers_t {
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

struct Meta {
    bit<1> b;
}

parser prs(packet_in p, out Headers_t headers, inout Meta meta, inout standard_metadata std_meta) {
    state start {
        p.extract(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800: ip;
            default: reject;
        }
    }
    state ip {
        p.extract(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout Meta meta, inout standard_metadata unused) {
    action Reject(IPv4Address add) {
        headers.ipv4.srcAddr = add;
    }
    table Check_src_ip {
        key = {
            headers.ipv4.srcAddr: exact;
        }
        actions = {
            Reject;
            NoAction;
        }
        default_action = NoAction;
    }
    apply {
        if (!Check_src_ip.apply().hit) {
            meta.b = 1;
        }
        switch (Check_src_ip.apply().action_run) {
            Reject: {
                meta.b = 0;
            }
            NoAction: {
            }
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit(headers.ethernet);
        packet.emit(headers.ipv4);
    }
}

ubpf(prs(), pipe(), dprs()) main;

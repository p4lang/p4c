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
        p.extract<Ethernet_h>(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800: ip;
            default: reject;
        }
    }
    state ip {
        p.extract<IPv4_h>(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout Meta meta, inout standard_metadata unused) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("pipe.Reject") action Reject(@name("add") IPv4Address add) {
        headers.ipv4.srcAddr = add;
    }
    @name("pipe.Check_src_ip") table Check_src_ip_0 {
        key = {
            headers.ipv4.srcAddr: exact @name("headers.ipv4.srcAddr");
        }
        actions = {
            Reject();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        if (Check_src_ip_0.apply().hit) {
            ;
        } else {
            meta.b = 1w1;
        }
        switch (Check_src_ip_0.apply().action_run) {
            Reject: {
                meta.b = 1w0;
            }
            NoAction_1: {
            }
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    apply {
        packet.emit<Ethernet_h>(headers.ethernet);
        packet.emit<IPv4_h>(headers.ipv4);
    }
}

ubpf<Headers_t, Meta>(prs(), pipe(), dprs()) main;

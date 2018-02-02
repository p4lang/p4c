#include <core.p4>
#include <ebpf_model.p4>

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

parser prs(packet_in p, out Headers_t headers) {
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

control pipe(inout Headers_t headers, out bool pass) {
    @name(".NoAction") action NoAction_0() {
    }
    IPv4Address address;
    bool pass_1;
    @name("pipe.c1.Reject") action c1_Reject() {
        pass_1 = false;
    }
    @name("pipe.c1.Check_ip") table c1_Check_ip_0 {
        key = {
            address: exact @name("address") ;
        }
        actions = {
            c1_Reject();
            NoAction_0();
        }
        implementation = hash_table(32w1024);
        const default_action = NoAction_0();
    }
    apply {
        bool hasReturned_0 = false;
        if (!headers.ipv4.isValid()) {
            pass = false;
            hasReturned_0 = true;
        }
        if (!hasReturned_0) {
            address = headers.ipv4.srcAddr;
            c1_Check_ip_0.apply();
            pass = pass_1;
            address = headers.ipv4.dstAddr;
            c1_Check_ip_0.apply();
            pass = pass_1;
        }
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;


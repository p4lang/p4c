#include "/home/cdodd/p4c/build/../p4include/core.p4"
#include "/home/cdodd/p4c/build/../p4include/ebpf_model.p4"

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

struct Headers_t {
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

parser prs(packet_in p, out Headers_t headers) {
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

control pipe(inout Headers_t headers, out bool pass) {
    action Reject() {
        bool hasReturned_0 = false;
        pass = false;
    }
    table Check_ip(in IPv4Address address) {
        key = {
            address: exact;
        }
        actions = {
            Reject;
            NoAction;
        }
        implementation = hash_table(32w1024);
        const default_action = NoAction;
    }

    apply {
        bool hasExited = false;
        @name("hasReturned_0") bool hasReturned_0_0;
        hasReturned_0_0 = false;
        pass = true;
        if (!headers.ipv4.isValid()) {
            pass = false;
            hasReturned_0_0 = true;
            ;
        }
        if (!hasReturned_0_0) {
            Check_ip.apply(headers.ipv4.srcAddr);
            Check_ip.apply(headers.ipv4.dstAddr);
        }
    }
}

ebpfFilter(prs(), pipe()) main;

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
    CounterArray(32w10, true) counters;
    action invalidate() {
        headers.ipv4.setInvalid();
        headers.ethernet.setInvalid();
    }
    table t {
        actions = {
            invalidate();
            @defaultonly NoAction();
        }
        implementation = array_table(32w1);
        default_action = NoAction();
    }
    apply {
        if (headers.ipv4.isValid()) {
            counters.increment(headers.ipv4.dstAddr);
            pass = true;
        }
        else {
            t.apply();
            pass = false;
        }
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;


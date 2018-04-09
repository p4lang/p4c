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
    bit<32> key_0;
    bit<32> key_1;
    @name("pipe.invalidate") action invalidate_0() {
        headers.ipv4.setInvalid();
        headers.ethernet.setInvalid();
        pass = true;
    }
    @name("pipe.drop") action drop_0() {
        pass = false;
    }
    @name("pipe.t") table t {
        key = {
            key_0                   : exact @name(" headers.ipv4.srcAddr") ;
            key_1                   : exact @name("headers.ipv4.dstAddr") ;
            headers.ethernet.dstAddr: exact @name("headers.ethernet.dstAddr") ;
            headers.ethernet.srcAddr: exact @name("headers.ethernet.srcAddr") ;
        }
        actions = {
            invalidate_0();
            drop_0();
        }
        implementation = hash_table(32w10);
        default_action = drop_0();
    }
    @hidden action act() {
        key_0 = headers.ipv4.srcAddr + 32w1;
        key_1 = headers.ipv4.dstAddr + 32w1;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
        t.apply();
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;


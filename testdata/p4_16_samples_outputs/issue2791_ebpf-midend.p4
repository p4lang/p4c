#include <core.p4>
#include <ebpf_model.p4>

header Ethernet_h {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header IPv4_h {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

struct Headers_t {
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract<Ethernet_h>(headers.ethernet);
        transition select(headers.ethernet.etherType, 16w1) {
            (16w0x800, 16w1): ip;
            default: reject;
        }
    }
    state ip {
        p.extract<IPv4_h>(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    @name("pipe.invalidate") action invalidate() {
        headers.ipv4.setInvalid();
        headers.ethernet.setInvalid();
        pass = true;
    }
    @name("pipe.drop") action drop() {
        pass = false;
    }
    @name("pipe.t") table t_0 {
        key = {
            headers.ipv4.srcAddr    : exact @name("headers.ipv4.srcAddr");
            headers.ipv4.dstAddr    : exact @name("headers.ipv4.dstAddr");
            headers.ethernet.dstAddr: exact @name("headers.ethernet.dstAddr");
            headers.ethernet.srcAddr: exact @name("headers.ethernet.srcAddr");
        }
        actions = {
            invalidate();
            drop();
        }
        implementation = hash_table(32w10);
        default_action = drop();
    }
    apply {
        t_0.apply();
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;

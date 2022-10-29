#include <core.p4>
#include <ebpf_model.p4>

header Ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header Ipv4_t {
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

header Tcp_t {
    bit<16> srcPort;
    bit<16> dstPort;
    bit<32> seqNo;
    bit<32> ackNo;
    bit<4>  dataOffset;
    bit<3>  res;
    bit<3>  ecn;
    bit<1>  urgent;
    bit<1>  ack;
    bit<1>  psh;
    bit<1>  rst;
    bit<1>  syn;
    bit<1>  fin;
    bit<16> window;
    bit<16> checksum;
    bit<16> urgentPtr;
}

struct Headers_t {
    Ethernet_t ethernet;
    Ipv4_t     ipv4;
    Tcp_t      tcp;
}

extern bool tcp_conntrack(in Headers_t hdrs);
parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract<Ethernet_t>(headers.ethernet);
        p.extract<Ipv4_t>(headers.ipv4);
        p.extract<Tcp_t>(headers.tcp);
        transition accept;
    }
}

control pipe(inout Headers_t headers, out bool pass) {
    @name("pipe.allow") bool allow_0;
    @hidden action ebpf_conntrack_extern77() {
        pass = true;
    }
    @hidden action ebpf_conntrack_extern75() {
        allow_0 = tcp_conntrack(headers);
    }
    @hidden action ebpf_conntrack_extern73() {
        pass = false;
    }
    @hidden table tbl_ebpf_conntrack_extern73 {
        actions = {
            ebpf_conntrack_extern73();
        }
        const default_action = ebpf_conntrack_extern73();
    }
    @hidden table tbl_ebpf_conntrack_extern75 {
        actions = {
            ebpf_conntrack_extern75();
        }
        const default_action = ebpf_conntrack_extern75();
    }
    @hidden table tbl_ebpf_conntrack_extern77 {
        actions = {
            ebpf_conntrack_extern77();
        }
        const default_action = ebpf_conntrack_extern77();
    }
    apply {
        tbl_ebpf_conntrack_extern73.apply();
        if (headers.tcp.isValid()) {
            tbl_ebpf_conntrack_extern75.apply();
            if (allow_0) {
                tbl_ebpf_conntrack_extern77.apply();
            }
        }
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;

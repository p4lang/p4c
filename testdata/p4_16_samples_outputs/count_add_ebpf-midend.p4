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
    @name("pipe.counters") CounterArray(32w10, true) counters_0;
    @hidden action count_add_ebpf54() {
        counters_0.add(headers.ipv4.dstAddr, (bit<32>)headers.ipv4.totalLen);
        pass = true;
    }
    @hidden action count_add_ebpf58() {
        pass = false;
    }
    @hidden table tbl_count_add_ebpf54 {
        actions = {
            count_add_ebpf54();
        }
        const default_action = count_add_ebpf54();
    }
    @hidden table tbl_count_add_ebpf58 {
        actions = {
            count_add_ebpf58();
        }
        const default_action = count_add_ebpf58();
    }
    apply {
        if (headers.ipv4.isValid()) {
            tbl_count_add_ebpf54.apply();
        } else {
            tbl_count_add_ebpf58.apply();
        }
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;

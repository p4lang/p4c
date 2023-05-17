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
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("pipe.counters") CounterArray(32w10, true) counters_0;
    @name("pipe.invalidate") action invalidate() {
        headers.ipv4.setInvalid();
        headers.ethernet.setInvalid();
    }
    @name("pipe.t") table t_0 {
        actions = {
            invalidate();
            @defaultonly NoAction_1();
        }
        implementation = array_table(32w1);
        default_action = NoAction_1();
    }
    @hidden action valid_ebpf49() {
        counters_0.increment(headers.ipv4.dstAddr);
        pass = true;
    }
    @hidden action valid_ebpf54() {
        pass = false;
    }
    @hidden table tbl_valid_ebpf49 {
        actions = {
            valid_ebpf49();
        }
        const default_action = valid_ebpf49();
    }
    @hidden table tbl_valid_ebpf54 {
        actions = {
            valid_ebpf54();
        }
        const default_action = valid_ebpf54();
    }
    apply {
        if (headers.ipv4.isValid()) {
            tbl_valid_ebpf49.apply();
        } else {
            t_0.apply();
            tbl_valid_ebpf54.apply();
        }
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;

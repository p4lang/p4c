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
    @name("pipe.act_pass") action act_pass() {
        pass = true;
    }
    @name("pipe.Check_src_ip") table Check_src_ip_0 {
        key = {
            headers.ipv4.srcAddr: ternary @name("headers.ipv4.srcAddr");
        }
        actions = {
            act_pass();
            NoAction_1();
        }
        implementation = hash_table(32w1024);
        const default_action = act_pass();
    }
    @hidden action ternary_ebpf64() {
        pass = false;
    }
    @hidden table tbl_ternary_ebpf64 {
        actions = {
            ternary_ebpf64();
        }
        const default_action = ternary_ebpf64();
    }
    apply {
        tbl_ternary_ebpf64.apply();
        Check_src_ip_0.apply();
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;

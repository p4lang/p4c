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
    @name("pipe.address_0") bit<32> address_0;
    @name("pipe.pass_0") bool pass_0;
    @name("pipe.hasReturned") bool hasReturned;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("pipe.c1.Reject") action c1_Reject_0() {
        pass_0 = false;
    }
    @name("pipe.c1.Check_ip") table c1_Check_ip {
        key = {
            address_0: exact @name("address");
        }
        actions = {
            c1_Reject_0();
            NoAction_1();
        }
        implementation = hash_table(32w1024);
        const default_action = NoAction_1();
    }
    @hidden action two_ebpf69() {
        pass = false;
        hasReturned = true;
    }
    @hidden action two_ebpf66() {
        hasReturned = false;
        pass = true;
    }
    @hidden action act() {
        address_0 = headers.ipv4.srcAddr;
        pass_0 = pass;
    }
    @hidden action act_0() {
        pass = pass_0;
        address_0 = headers.ipv4.dstAddr;
    }
    @hidden action act_1() {
        pass = pass_0;
    }
    @hidden table tbl_two_ebpf66 {
        actions = {
            two_ebpf66();
        }
        const default_action = two_ebpf66();
    }
    @hidden table tbl_two_ebpf69 {
        actions = {
            two_ebpf69();
        }
        const default_action = two_ebpf69();
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_act_0 {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_1 {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    apply {
        tbl_two_ebpf66.apply();
        if (headers.ipv4.isValid()) {
            ;
        } else {
            tbl_two_ebpf69.apply();
        }
        if (hasReturned) {
            ;
        } else {
            tbl_act.apply();
            c1_Check_ip.apply();
            tbl_act_0.apply();
            c1_Check_ip.apply();
            tbl_act_1.apply();
        }
    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;

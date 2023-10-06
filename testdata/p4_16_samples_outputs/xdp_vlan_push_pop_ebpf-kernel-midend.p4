#include <core.p4>
#include <ebpf_model.p4>
#include <xdp_model.p4>

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

header VLAN_h {
    bit<3>  pcp;
    bit<1>  dei;
    bit<12> vid;
    bit<16> tpid;
}

struct Headers_t {
    Ethernet_h ethernet;
    VLAN_h     vlan;
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract<Ethernet_h>(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x8100: vlan;
            default: accept;
        }
    }
    state vlan {
        p.extract<VLAN_h>(headers.vlan);
        transition accept;
    }
}

control sw(inout Headers_t headers, in xdp_input imd, out xdp_output omd) {
    @hidden action xdp_vlan_push_pop_ebpfkernel36() {
        headers.ethernet.etherType = headers.vlan.tpid;
        headers.vlan.setInvalid();
    }
    @hidden action xdp_vlan_push_pop_ebpfkernel39() {
        headers.vlan.setValid();
        headers.vlan.tpid = headers.ethernet.etherType;
        headers.ethernet.etherType = 16w0x8100;
        headers.vlan.pcp = 3w0;
        headers.vlan.dei = 1w0;
        headers.vlan.vid = 12w42;
    }
    @hidden action xdp_vlan_push_pop_ebpfkernel47() {
        omd.output_action = xdp_action.XDP_PASS;
        omd.output_port = imd.input_port;
    }
    @hidden table tbl_xdp_vlan_push_pop_ebpfkernel36 {
        actions = {
            xdp_vlan_push_pop_ebpfkernel36();
        }
        const default_action = xdp_vlan_push_pop_ebpfkernel36();
    }
    @hidden table tbl_xdp_vlan_push_pop_ebpfkernel39 {
        actions = {
            xdp_vlan_push_pop_ebpfkernel39();
        }
        const default_action = xdp_vlan_push_pop_ebpfkernel39();
    }
    @hidden table tbl_xdp_vlan_push_pop_ebpfkernel47 {
        actions = {
            xdp_vlan_push_pop_ebpfkernel47();
        }
        const default_action = xdp_vlan_push_pop_ebpfkernel47();
    }
    apply {
        if (headers.vlan.isValid()) {
            tbl_xdp_vlan_push_pop_ebpfkernel36.apply();
        } else {
            tbl_xdp_vlan_push_pop_ebpfkernel39.apply();
        }
        tbl_xdp_vlan_push_pop_ebpfkernel47.apply();
    }
}

control deprs(in Headers_t headers, packet_out packet) {
    @hidden action xdp_vlan_push_pop_ebpfkernel55() {
        packet.emit<Ethernet_h>(headers.ethernet);
        packet.emit<VLAN_h>(headers.vlan);
    }
    @hidden table tbl_xdp_vlan_push_pop_ebpfkernel55 {
        actions = {
            xdp_vlan_push_pop_ebpfkernel55();
        }
        const default_action = xdp_vlan_push_pop_ebpfkernel55();
    }
    apply {
        tbl_xdp_vlan_push_pop_ebpfkernel55.apply();
    }
}

xdp<Headers_t>(prs(), sw(), deprs()) main;

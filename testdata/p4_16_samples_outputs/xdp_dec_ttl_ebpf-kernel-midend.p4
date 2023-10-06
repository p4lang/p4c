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

struct Headers_t {
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract<Ethernet_h>(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800: ipv4;
            default: accept;
        }
    }
    state ipv4 {
        p.extract<IPv4_h>(headers.ipv4);
        transition accept;
    }
}

control sw(inout Headers_t headers, in xdp_input imd, out xdp_output omd) {
    @hidden action xdp_dec_ttl_ebpfkernel29() {
        headers.ipv4.ttl = headers.ipv4.ttl + 8w255;
    }
    @hidden action xdp_dec_ttl_ebpfkernel32() {
        omd.output_action = xdp_action.XDP_PASS;
        omd.output_port = imd.input_port;
    }
    @hidden table tbl_xdp_dec_ttl_ebpfkernel29 {
        actions = {
            xdp_dec_ttl_ebpfkernel29();
        }
        const default_action = xdp_dec_ttl_ebpfkernel29();
    }
    @hidden table tbl_xdp_dec_ttl_ebpfkernel32 {
        actions = {
            xdp_dec_ttl_ebpfkernel32();
        }
        const default_action = xdp_dec_ttl_ebpfkernel32();
    }
    apply {
        if (headers.ipv4.isValid()) {
            tbl_xdp_dec_ttl_ebpfkernel29.apply();
        }
        tbl_xdp_dec_ttl_ebpfkernel32.apply();
    }
}

control deprs(in Headers_t headers, packet_out packet) {
    @hidden action xdp_dec_ttl_ebpfkernel40() {
        packet.emit<Ethernet_h>(headers.ethernet);
        packet.emit<IPv4_h>(headers.ipv4);
    }
    @hidden table tbl_xdp_dec_ttl_ebpfkernel40 {
        actions = {
            xdp_dec_ttl_ebpfkernel40();
        }
        const default_action = xdp_dec_ttl_ebpfkernel40();
    }
    apply {
        tbl_xdp_dec_ttl_ebpfkernel40.apply();
    }
}

xdp<Headers_t>(prs(), sw(), deprs()) main;

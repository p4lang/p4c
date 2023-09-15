#include <core.p4>
#include <ebpf_model.p4>
#include <xdp_model.p4>

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
    apply {
        if (headers.vlan.isValid()) {
            headers.ethernet.etherType = headers.vlan.tpid;
            headers.vlan.setInvalid();
        } else {
            headers.vlan.setValid();
            headers.vlan.tpid = headers.ethernet.etherType;
            headers.ethernet.etherType = 16w0x8100;
            headers.vlan.pcp = 3w0;
            headers.vlan.dei = 1w0;
            headers.vlan.vid = 12w42;
        }
        omd.output_action = xdp_action.XDP_PASS;
        omd.output_port = imd.input_port;
    }
}

control deprs(in Headers_t headers, packet_out packet) {
    apply {
        packet.emit<Ethernet_h>(headers.ethernet);
        packet.emit<VLAN_h>(headers.vlan);
    }
}

xdp<Headers_t>(prs(), sw(), deprs()) main;

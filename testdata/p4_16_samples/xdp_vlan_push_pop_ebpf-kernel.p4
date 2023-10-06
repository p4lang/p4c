#include <xdp_model.p4>
#include <core.p4>

#include "ebpf_headers.p4"

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
        p.extract(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x8100 : vlan;
            default   : accept;
        }
    }

    state vlan {
        p.extract(headers.vlan);
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
            headers.ethernet.etherType = 0x8100;
            headers.vlan.pcp  = 0;
            headers.vlan.dei  = 0;
            headers.vlan.vid  = 42;
        }

        omd.output_action = xdp_action.XDP_PASS;
        omd.output_port   = imd.input_port;
    }
}

control deprs(in Headers_t headers, packet_out packet)
{
    apply {
        packet.emit(headers.ethernet);
        packet.emit(headers.vlan);
    }
}

xdp(prs(), sw(), deprs()) main;

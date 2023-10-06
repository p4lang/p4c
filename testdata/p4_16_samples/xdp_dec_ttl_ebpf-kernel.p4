#include <xdp_model.p4>
#include <core.p4>

#include "ebpf_headers.p4"

struct Headers_t {
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

parser prs(packet_in p, out Headers_t headers) {
    state start {
        p.extract(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x0800 : ipv4;
            default   : accept;
        }
    }

    state ipv4 {
        p.extract(headers.ipv4);
        transition accept;
    }
}

control sw(inout Headers_t headers, in xdp_input imd, out xdp_output omd) {
    apply {
        if (headers.ipv4.isValid()) {
          headers.ipv4.ttl = headers.ipv4.ttl - 1;
        }

        omd.output_action = xdp_action.XDP_PASS;
        omd.output_port   = imd.input_port;
    }
}

control deprs(in Headers_t headers, packet_out packet)
{
    apply {
        packet.emit(headers.ethernet);
        packet.emit(headers.ipv4);
    }
}

xdp(prs(), sw(), deprs()) main;

#include <core.p4>

parser parse<H>(packet_in packet, out H headers);
package ebpfFilter<H>(parse<H> prs);
header Ethernet_h {
    bit<64> dstAddr;
    bit<64> srcAddr;
    bit<16> etherType;
}

struct Headers_t {
    Ethernet_h ethernet;
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
        transition accept;
    }
}

ebpfFilter<Headers_t>(prs()) main;


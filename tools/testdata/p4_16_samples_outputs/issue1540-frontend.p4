#include <core.p4>

control Ingress<H, M>(inout H h, inout M m);
control IngressDeparser<H>(packet_out pkt, inout H h);
package Pipeline<H, M>(Ingress<H, M> g, IngressDeparser<H> _c);
package Top<H1, M1, H2, M2>(Pipeline<H1, M1> p0, Pipeline<H2, M2> p1);
header header_t {
}

struct metadata_t {
}

control SwitchIngress(inout header_t t, inout metadata_t m) {
    apply {
    }
}

control SwitchIngressDeparser(packet_out pkt, inout header_t h) {
    apply {
    }
}

Pipeline<header_t, metadata_t>(SwitchIngress(), SwitchIngressDeparser()) p0;

Pipeline<header_t, metadata_t>(SwitchIngress(), SwitchIngressDeparser()) p1;

Top<header_t, metadata_t, header_t, metadata_t>(p0, p1) main;


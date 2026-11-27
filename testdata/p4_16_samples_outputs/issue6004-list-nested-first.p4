#include <core.p4>

header H {
    bit<8> value;
}

control Deparser(packet_out packet, in H hdr1, in H hdr2, in H hdr3) {
    apply {
        packet.emit<tuple<H, tuple<H, H>>>({ hdr1, { hdr2, hdr3 } });
    }
}

control D(packet_out packet, in H hdr1, in H hdr2, in H hdr3);
package Test(D deparser);
Test(Deparser()) main;

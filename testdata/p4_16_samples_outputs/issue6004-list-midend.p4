#include <core.p4>

header H {
    bit<8> value;
}

control Deparser(packet_out packet, in H hdr1, in H hdr2) {
    @hidden action issue6004list12() {
        packet.emit<H>(hdr1);
        packet.emit<H>(hdr2);
    }
    @hidden table tbl_issue6004list12 {
        actions = {
            issue6004list12();
        }
        const default_action = issue6004list12();
    }
    apply {
        tbl_issue6004list12.apply();
    }
}

control D(packet_out packet, in H hdr1, in H hdr2);
package Test(D deparser);
Test(Deparser()) main;

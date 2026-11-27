#include <core.p4>

header H {
    bit<8> value;
}

control Deparser(packet_out packet, in H hdr1, in H hdr2, in H hdr3) {
    @hidden action issue6004listnested12() {
        packet.emit<H>(hdr1);
        packet.emit<H>(hdr2);
        packet.emit<H>(hdr3);
    }
    @hidden table tbl_issue6004listnested12 {
        actions = {
            issue6004listnested12();
        }
        const default_action = issue6004listnested12();
    }
    apply {
        tbl_issue6004listnested12.apply();
    }
}

control D(packet_out packet, in H hdr1, in H hdr2, in H hdr3);
package Test(D deparser);
Test(Deparser()) main;

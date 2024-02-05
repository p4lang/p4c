#include <core.p4>

control Deparser(packet_out packet);
package SimpleArch(Deparser dep);
header hdr_t {
    bit<8> f0;
}

control MyDeparser_0(packet_out packet) {
    @hidden action issue4288l12() {
        packet.emit<hdr_t>((hdr_t){f0 = 8w0});
    }
    @hidden table tbl_issue4288l12 {
        actions = {
            issue4288l12();
        }
        const default_action = issue4288l12();
    }
    apply {
        tbl_issue4288l12.apply();
    }
}

SimpleArch(MyDeparser_0()) main;

#include <core.p4>

control Deparser(packet_out packet);
package SimpleArch(Deparser dep);
header hdr_t {
    bit<8> f0;
}

control MyDeparser_0(packet_out packet) {
    apply {
        packet.emit<hdr_t>((hdr_t){f0 = 8w0});
    }
}

SimpleArch(MyDeparser_0()) main;

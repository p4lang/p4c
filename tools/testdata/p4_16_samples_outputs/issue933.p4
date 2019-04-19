#include <core.p4>

control Deparser<H>(packet_out packet, in H hdr);
package Switch<H>(Deparser<H> d);
struct headers {
}

control MyDeparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<headers>({  });
    }
}

Switch(MyDeparser()) main;


#include <core.p4>
#include <v1model.p4>
header h_t {
     bit<8> x;
}
struct headers {
    h_t h;
}
struct metadata { }
parser p(packet_in pkt,out headers hdr,inout metadata meta,inout standard_metadata_t std_meta) {
    state start {
        transition accept;
    }
}
control c1(inout headers hdr,inout metadata meta) {
    apply { }
}
control c2(inout headers hdr,inout metadata meta,inout standard_metadata_t std_meta) {
    apply { }
}
control c3(inout headers hdr,inout metadata meta,inout standard_metadata_t std_meta) {
    action a(in bool b) {
        hdr.h.x = 8w0;
    }
    table t {
        key = { }
        actions = {
            a(hdr.h.isValid() || (bool) 1w1);
        }
    }
    apply {
        t.apply();
    }
}
control c4(inout headers hdr,inout metadata meta) {
    apply { }
}
control c5(packet_out pkt,in headers hdr) {
    apply { }
}
V1Switch(p(),c1(),c2(),c3(),c4(),c5()) main;

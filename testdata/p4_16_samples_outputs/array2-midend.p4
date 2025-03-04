#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header data_t {
    bit<32>    f1;
    bit<32>    f2;
    bit<16>    h1;
    bit<16>    h2;
    bit<8>[16] arr;
}

struct metadata {
}

struct headers {
    data_t data;
}

parser p(packet_in b, out headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    state start {
        b.extract<data_t>(hdr.data);
        transition accept;
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    @hidden action array2l32() {
        hdr.data.arr[0] = hdr.data.f1[7:0];
        hdr.data.arr[1] = hdr.data.f1[15:8];
        hdr.data.arr[2] = hdr.data.f1[23:16];
        hdr.data.arr[3] = hdr.data.f1[31:24];
        hdr.data.arr[4] = hdr.data.f2[7:0];
        hdr.data.arr[5] = hdr.data.f2[15:8];
        hdr.data.arr[6] = hdr.data.f2[23:16];
        hdr.data.arr[7] = hdr.data.f2[31:24];
    }
    @hidden table tbl_array2l32 {
        actions = {
            array2l32();
        }
        const default_action = array2l32();
    }
    apply {
        tbl_array2l32.apply();
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t stdmeta) {
    apply {
    }
}

control vc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control uc(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control deparser(packet_out packet, in headers hdr) {
    apply {
        packet.emit<data_t>(hdr.data);
    }
}

V1Switch<headers, metadata>(p(), vc(), ingress(), egress(), uc(), deparser()) main;

#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<16> result;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {
}

extern Checksum {
    Checksum();
    void add<T>(in T data);
    void subtract<T>(in T data);
    bool verify();
    bit<16> get();
    bit<16> update<T>(in T data, @optional in bool zeros_as_ones);
}

parser p(packet_in pkt, out Headers hdr, inout Meta m, inout standard_metadata_t sm) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h);
        transition accept;
    }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    apply {
    }
}

control vrfy(inout Headers h, inout Meta m) {
    apply {
    }
}

control update(inout Headers h, inout Meta m) {
    apply {
    }
}

struct tuple_0 {
    bit<48> f0;
    bit<48> f1;
    bit<16> f2;
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t sm) {
    @name("egress.ipv4_checksum") Checksum() ipv4_checksum_0;
    @hidden action issue2664bmv2l60() {
        h.h.result = ipv4_checksum_0.update<tuple_0>((tuple_0){f0 = h.eth_hdr.dst_addr,f1 = h.eth_hdr.src_addr,f2 = h.eth_hdr.eth_type});
    }
    @hidden table tbl_issue2664bmv2l60 {
        actions = {
            issue2664bmv2l60();
        }
        const default_action = issue2664bmv2l60();
    }
    apply {
        tbl_issue2664bmv2l60.apply();
    }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit<ethernet_t>(h.eth_hdr);
        pkt.emit<H>(h.h);
    }
}

V1Switch<Headers, Meta>(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

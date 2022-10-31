#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> eth_type;
}

header H {
    bit<8> a;
    bit<8> b;
    bool c;
    bit<7> pad;
}

struct Headers {
    ethernet_t eth_hdr;
    H          h;
}

struct Meta {}

parser p(packet_in pkt, out Headers h, inout Meta meta, inout standard_metadata_t stdmeta) {
    state start {
        pkt.extract(h.eth_hdr);
        transition parse_h;
    }
    state parse_h {
        pkt.extract(h.h);
        transition accept;
    }
}

control vrfy(inout Headers h, inout Meta meta) {
  apply { }
}

control ingress(inout Headers h, inout Meta m, inout standard_metadata_t s) {
    apply {
        h.h.a = 8w12;
        h.h.b = 8w16;
        h.h.c = true;
        log_msg("Test message 1");
        log_msg("Test message 2. h.h.a ={}", {h.h.a});
        log_msg<tuple<bit<8>>>("Test message 2. h.h.a ={}", {h.h.a});
        log_msg("Test message 3. h.h.c ={}", {h.h.c});
        log_msg("Test message 4. h.h.a = {} , h.h.b = {} ", {h.h.a, h.h.b});
    }
}

control egress(inout Headers h, inout Meta m, inout standard_metadata_t s) {
    apply { }
}

control update(inout Headers h, inout Meta m) {
    apply { }
}

control deparser(packet_out pkt, in Headers h) {
    apply {
        pkt.emit(h);
    }
}

V1Switch(p(), vrfy(), ingress(), egress(), update(), deparser()) main;

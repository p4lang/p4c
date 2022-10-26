#include <core.p4>

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

extern Checksum {
    Checksum();
    void add<T>(in T data);
    void subtract<T>(in T data);
    bool verify();
    bit<16> get();
    bit<16> update<T>(in T data, @optional in bool zeros_as_ones);
}

parser p(packet_in pkt, out Headers hdr) {
    state start {
        pkt.extract<ethernet_t>(hdr.eth_hdr);
        pkt.extract<H>(hdr.h);
        transition accept;
    }
}

control ingress(inout Headers h) {
    apply {
    }
}

struct tuple_0 {
    bit<48> f0;
    bit<48> f1;
    bit<16> f2;
}

control egress(inout Headers h) {
    @name("egress.ipv4_checksum") Checksum() ipv4_checksum_0;
    @hidden action gauntlet_optionalbmv2l45() {
        h.h.result = ipv4_checksum_0.update<tuple_0>((tuple_0){f0 = h.eth_hdr.dst_addr,f1 = h.eth_hdr.src_addr,f2 = h.eth_hdr.eth_type});
    }
    @hidden table tbl_gauntlet_optionalbmv2l45 {
        actions = {
            gauntlet_optionalbmv2l45();
        }
        const default_action = gauntlet_optionalbmv2l45();
    }
    apply {
        tbl_gauntlet_optionalbmv2l45.apply();
    }
}

control deparser(packet_out pkt, in Headers h) {
    @hidden action gauntlet_optionalbmv2l51() {
        pkt.emit<ethernet_t>(h.eth_hdr);
        pkt.emit<H>(h.h);
    }
    @hidden table tbl_gauntlet_optionalbmv2l51 {
        actions = {
            gauntlet_optionalbmv2l51();
        }
        const default_action = gauntlet_optionalbmv2l51();
    }
    apply {
        tbl_gauntlet_optionalbmv2l51.apply();
    }
}

parser Parser(packet_in b, out Headers hdr);
control Ingress(inout Headers hdr);
control Egress(inout Headers hdr);
control Deparser(packet_out b, in Headers hdr);
package top(Parser p, Ingress ig, Egress eg, Deparser dep);
top(p(), ingress(), egress(), deparser()) main;

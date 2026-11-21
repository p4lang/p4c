#include <core.p4>
#include <tc/pna.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> kind;
}

header mode_t {
    bit<8> value;
}

header vlan_t {
    bit<3>  pcp;
    bit<1>  dei;
    bit<12> vid;
    bit<16> ether_type;
}

header inner_t {
    bit<16> tci;
    bit<16> ether_type;
}

struct headers_t {
    ethernet_t ethernet;
    inner_t    inner;
    mode_t     mode;
    vlan_t     vlan;
}

struct metadata_t {
    bit<1>  present;
    bit<16> tci;
    bit<16> proto;
    bit<16> kind;
    bit<8>  mode;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.kind) {
            16w0x8100: parse_inner;
            16w0x88a8: parse_inner;
            default: parse_mode;
        }
    }
    state parse_inner {
        pkt.extract<inner_t>(hdr.inner);
        transition parse_mode;
    }
    state parse_mode {
        pkt.extract<mode_t>(hdr.mode);
        meta.present = istd.vlan_present;
        meta.tci = istd.vlan_tci;
        meta.proto = istd.vlan_proto;
        meta.kind = hdr.ethernet.kind;
        meta.mode = hdr.mode.value;
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    @name("Ingress.tci") bit<16> tci_0;
    @name("Ingress.proto") bit<16> proto_0;
    @name("Ingress.tmp") bit<1> tmp;
    @hidden action tccodegenvlan53() {
        drop_packet();
    }
    @hidden action tccodegenvlan58() {
        skb_vlan_push(meta.proto, meta.tci);
        hdr.ethernet.kind = meta.proto;
        hdr.vlan.pcp = (bit<3>)(meta.tci >> 13);
        hdr.vlan.dei = (bit<1>)(meta.tci >> 12);
        hdr.vlan.vid = (bit<12>)meta.tci;
        hdr.vlan.ether_type = meta.kind;
        hdr.vlan.setValid();
    }
    @hidden action tccodegenvlan67() {
        skb_vlan_pop();
        hdr.ethernet.kind = meta.proto;
        hdr.vlan.pcp = (bit<3>)(meta.tci >> 13);
        hdr.vlan.dei = (bit<1>)(meta.tci >> 12);
        hdr.vlan.vid = (bit<12>)meta.tci;
        hdr.vlan.ether_type = meta.kind;
        hdr.vlan.setValid();
    }
    @hidden action tccodegenvlan78() {
        tci_0 = skb_get_vlan_tci();
        proto_0 = skb_get_vlan_proto();
        skb_vlan_pop();
        hdr.ethernet.kind = proto_0;
        hdr.vlan.pcp = (bit<3>)(tci_0 >> 13);
        hdr.vlan.dei = (bit<1>)(tci_0 >> 12);
        hdr.vlan.vid = (bit<12>)tci_0;
        hdr.vlan.ether_type = meta.kind;
        hdr.vlan.setValid();
    }
    @hidden action tccodegenvlan77() {
        tmp = skb_get_vlan_present();
    }
    @hidden action tccodegenvlan90() {
        skb_vlan_push(16w0x8100, 16w0x64);
    }
    @hidden action tccodegenvlan97() {
        hdr.ethernet.kind = hdr.inner.ether_type;
        hdr.inner.setInvalid();
    }
    @hidden action tccodegenvlan95() {
        skb_vlan_pop();
    }
    @hidden action tccodegenvlan101() {
        send_to_port(32w1);
    }
    @hidden table tbl_tccodegenvlan53 {
        actions = {
            tccodegenvlan53();
        }
        const default_action = tccodegenvlan53();
    }
    @hidden table tbl_tccodegenvlan58 {
        actions = {
            tccodegenvlan58();
        }
        const default_action = tccodegenvlan58();
    }
    @hidden table tbl_tccodegenvlan67 {
        actions = {
            tccodegenvlan67();
        }
        const default_action = tccodegenvlan67();
    }
    @hidden table tbl_tccodegenvlan77 {
        actions = {
            tccodegenvlan77();
        }
        const default_action = tccodegenvlan77();
    }
    @hidden table tbl_tccodegenvlan78 {
        actions = {
            tccodegenvlan78();
        }
        const default_action = tccodegenvlan78();
    }
    @hidden table tbl_tccodegenvlan90 {
        actions = {
            tccodegenvlan90();
        }
        const default_action = tccodegenvlan90();
    }
    @hidden table tbl_tccodegenvlan95 {
        actions = {
            tccodegenvlan95();
        }
        const default_action = tccodegenvlan95();
    }
    @hidden table tbl_tccodegenvlan97 {
        actions = {
            tccodegenvlan97();
        }
        const default_action = tccodegenvlan97();
    }
    @hidden table tbl_tccodegenvlan101 {
        actions = {
            tccodegenvlan101();
        }
        const default_action = tccodegenvlan101();
    }
    apply {
        if (istd.parser_error != error.NoError) {
            tbl_tccodegenvlan53.apply();
        } else {
            if (meta.present == 1w1 && meta.mode == 8w1) {
                tbl_tccodegenvlan58.apply();
            } else if (meta.present == 1w1 && meta.mode == 8w2) {
                tbl_tccodegenvlan67.apply();
            } else if (meta.present == 1w1 && meta.mode == 8w4) {
                tbl_tccodegenvlan77.apply();
                if (tmp == 1w1) {
                    tbl_tccodegenvlan78.apply();
                }
            } else if (meta.present == 1w0 && meta.mode == 8w3) {
                tbl_tccodegenvlan90.apply();
            } else if (meta.mode == 8w5) {
                tbl_tccodegenvlan95.apply();
                if (hdr.inner.isValid()) {
                    tbl_tccodegenvlan97.apply();
                }
            }
            tbl_tccodegenvlan101.apply();
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action tccodegenvlan107() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<inner_t>(hdr.inner);
        pkt.emit<vlan_t>(hdr.vlan);
        pkt.emit<mode_t>(hdr.mode);
    }
    @hidden table tbl_tccodegenvlan107 {
        actions = {
            tccodegenvlan107();
        }
        const default_action = tccodegenvlan107();
    }
    apply {
        tbl_tccodegenvlan107.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;

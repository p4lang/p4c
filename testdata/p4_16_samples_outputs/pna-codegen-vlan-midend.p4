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
    @hidden action pnacodegenvlan53() {
        drop_packet();
    }
    @hidden action pnacodegenvlan58() {
        skb_vlan_push(meta.proto, meta.tci);
        hdr.ethernet.kind = meta.proto;
        hdr.vlan.pcp = (bit<3>)(meta.tci >> 13);
        hdr.vlan.dei = (bit<1>)(meta.tci >> 12);
        hdr.vlan.vid = (bit<12>)meta.tci;
        hdr.vlan.ether_type = meta.kind;
        hdr.vlan.setValid();
    }
    @hidden action pnacodegenvlan67() {
        skb_vlan_pop();
        hdr.ethernet.kind = meta.proto;
        hdr.vlan.pcp = (bit<3>)(meta.tci >> 13);
        hdr.vlan.dei = (bit<1>)(meta.tci >> 12);
        hdr.vlan.vid = (bit<12>)meta.tci;
        hdr.vlan.ether_type = meta.kind;
        hdr.vlan.setValid();
    }
    @hidden action pnacodegenvlan78() {
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
    @hidden action pnacodegenvlan77() {
        tmp = skb_get_vlan_present();
    }
    @hidden action pnacodegenvlan90() {
        skb_vlan_push(16w0x8100, 16w0x64);
    }
    @hidden action pnacodegenvlan97() {
        hdr.ethernet.kind = hdr.inner.ether_type;
        hdr.inner.setInvalid();
    }
    @hidden action pnacodegenvlan95() {
        skb_vlan_pop();
    }
    @hidden action pnacodegenvlan101() {
        send_to_port(32w1);
    }
    @hidden table tbl_pnacodegenvlan53 {
        actions = {
            pnacodegenvlan53();
        }
        const default_action = pnacodegenvlan53();
    }
    @hidden table tbl_pnacodegenvlan58 {
        actions = {
            pnacodegenvlan58();
        }
        const default_action = pnacodegenvlan58();
    }
    @hidden table tbl_pnacodegenvlan67 {
        actions = {
            pnacodegenvlan67();
        }
        const default_action = pnacodegenvlan67();
    }
    @hidden table tbl_pnacodegenvlan77 {
        actions = {
            pnacodegenvlan77();
        }
        const default_action = pnacodegenvlan77();
    }
    @hidden table tbl_pnacodegenvlan78 {
        actions = {
            pnacodegenvlan78();
        }
        const default_action = pnacodegenvlan78();
    }
    @hidden table tbl_pnacodegenvlan90 {
        actions = {
            pnacodegenvlan90();
        }
        const default_action = pnacodegenvlan90();
    }
    @hidden table tbl_pnacodegenvlan95 {
        actions = {
            pnacodegenvlan95();
        }
        const default_action = pnacodegenvlan95();
    }
    @hidden table tbl_pnacodegenvlan97 {
        actions = {
            pnacodegenvlan97();
        }
        const default_action = pnacodegenvlan97();
    }
    @hidden table tbl_pnacodegenvlan101 {
        actions = {
            pnacodegenvlan101();
        }
        const default_action = pnacodegenvlan101();
    }
    apply {
        if (istd.parser_error != error.NoError) {
            tbl_pnacodegenvlan53.apply();
        } else {
            if (meta.present == 1w1 && meta.mode == 8w1) {
                tbl_pnacodegenvlan58.apply();
            } else if (meta.present == 1w1 && meta.mode == 8w2) {
                tbl_pnacodegenvlan67.apply();
            } else if (meta.present == 1w1 && meta.mode == 8w4) {
                tbl_pnacodegenvlan77.apply();
                if (tmp == 1w1) {
                    tbl_pnacodegenvlan78.apply();
                }
            } else if (meta.present == 1w0 && meta.mode == 8w3) {
                tbl_pnacodegenvlan90.apply();
            } else if (meta.mode == 8w5) {
                tbl_pnacodegenvlan95.apply();
                if (hdr.inner.isValid()) {
                    tbl_pnacodegenvlan97.apply();
                }
            }
            tbl_pnacodegenvlan101.apply();
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    @hidden action pnacodegenvlan107() {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<inner_t>(hdr.inner);
        pkt.emit<vlan_t>(hdr.vlan);
        pkt.emit<mode_t>(hdr.mode);
    }
    @hidden table tbl_pnacodegenvlan107 {
        actions = {
            pnacodegenvlan107();
        }
        const default_action = pnacodegenvlan107();
    }
    apply {
        tbl_pnacodegenvlan107.apply();
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;

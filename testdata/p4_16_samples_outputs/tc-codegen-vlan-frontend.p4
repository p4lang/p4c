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
    @name("Ingress.tmp_0") bool tmp_0;
    apply {
        if (istd.parser_error != error.NoError) {
            drop_packet();
        } else {
            if (meta.present == 1w1 && meta.mode == 8w1) {
                skb_vlan_push(meta.proto, meta.tci);
                hdr.ethernet.kind = meta.proto;
                hdr.vlan.pcp = (bit<3>)(meta.tci >> 13);
                hdr.vlan.dei = (bit<1>)(meta.tci >> 12);
                hdr.vlan.vid = (bit<12>)meta.tci;
                hdr.vlan.ether_type = meta.kind;
                hdr.vlan.setValid();
            } else if (meta.present == 1w1 && meta.mode == 8w2) {
                skb_vlan_pop();
                hdr.ethernet.kind = meta.proto;
                hdr.vlan.pcp = (bit<3>)(meta.tci >> 13);
                hdr.vlan.dei = (bit<1>)(meta.tci >> 12);
                hdr.vlan.vid = (bit<12>)meta.tci;
                hdr.vlan.ether_type = meta.kind;
                hdr.vlan.setValid();
            } else if (meta.present == 1w1 && meta.mode == 8w4) {
                tmp = skb_get_vlan_present();
                tmp_0 = tmp == 1w1;
                if (tmp_0) {
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
            } else if (meta.present == 1w0 && meta.mode == 8w3) {
                skb_vlan_push(16w0x8100, 16w0x64);
            } else if (meta.mode == 8w5) {
                skb_vlan_pop();
                if (hdr.inner.isValid()) {
                    hdr.ethernet.kind = hdr.inner.ether_type;
                    hdr.inner.setInvalid();
                }
            }
            send_to_port((PortId_t)32w1);
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<inner_t>(hdr.inner);
        pkt.emit<vlan_t>(hdr.vlan);
        pkt.emit<mode_t>(hdr.mode);
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;

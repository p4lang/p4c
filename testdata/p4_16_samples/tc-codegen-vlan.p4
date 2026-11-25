/*
 * SPDX-FileCopyrightText: 2026 Mojatatu Networks
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <core.p4>
#include <tc/pna.p4>

/* The mode header is a whole byte because the backend reads a bit<48> field
 * as a byte array, so a slice of it is not an arithmetic value. */
header ethernet_t { bit<48> dst; bit<48> src; bit<16> kind; }
header mode_t { bit<8> value; }
header vlan_t { bit<3> pcp; bit<1> dei; bit<12> vid; bit<16> ether_type; }
header inner_t { bit<16> tci; bit<16> ether_type; }
struct headers_t { ethernet_t ethernet; inner_t inner; mode_t mode; vlan_t vlan; }
struct metadata_t {
    bit<1> present;
    bit<16> tci;
    bit<16> proto;
    bit<16> kind;
    bit<8> mode;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta,
              in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract(hdr.ethernet);
        transition select(hdr.ethernet.kind) {
            0x8100: parse_inner;
            0x88a8: parse_inner;
            default: parse_mode;
        }
    }
    /* A tag the stack left in the frame, behind the MAC addresses. */
    state parse_inner {
        pkt.extract(hdr.inner);
        transition parse_mode;
    }
    state parse_mode {
        pkt.extract(hdr.mode);
        meta.present = istd.vlan_present;
        meta.tci = istd.vlan_tci;
        meta.proto = istd.vlan_proto;
        meta.kind = hdr.ethernet.kind;
        meta.mode = hdr.mode.value;
        transition accept;
    }
}
control Ingress(inout headers_t hdr, inout metadata_t meta,
                in pna_main_input_metadata_t istd,
                inout pna_main_output_metadata_t ostd) {
    apply {
        if (istd.parser_error != error.NoError) { drop_packet(); return; }
        if (meta.present == 1 && meta.mode == 1) {
            /* The tag arrives in metadata.  Re-state it as the stack tag so
             * the helper writes the old one into the frame; the same tag
             * comes back in metadata. */
            skb_vlan_push(meta.proto, meta.tci);
            hdr.ethernet.kind = meta.proto;
            hdr.vlan.pcp = (bit<3>)(meta.tci >> 13);
            hdr.vlan.dei = (bit<1>)(meta.tci >> 12);
            hdr.vlan.vid = (bit<12>)meta.tci;
            hdr.vlan.ether_type = meta.kind;
            hdr.vlan.setValid();
        } else if (meta.present == 1 && meta.mode == 2) {
            /* Drop the metadata tag and write it into the frame instead. */
            skb_vlan_pop();
            hdr.ethernet.kind = meta.proto;
            hdr.vlan.pcp = (bit<3>)(meta.tci >> 13);
            hdr.vlan.dei = (bit<1>)(meta.tci >> 12);
            hdr.vlan.vid = (bit<12>)meta.tci;
            hdr.vlan.ether_type = meta.kind;
            hdr.vlan.setValid();
        } else if (meta.present == 1 && meta.mode == 4) {
            /* Same result, but read the tag through the control accessors.
             * The pop clears the metadata, so the tag is copied out first. */
            if (skb_get_vlan_present() == 1) {
                bit<16> tci = skb_get_vlan_tci();
                bit<16> proto = skb_get_vlan_proto();
                skb_vlan_pop();
                hdr.ethernet.kind = proto;
                hdr.vlan.pcp = (bit<3>)(tci >> 13);
                hdr.vlan.dei = (bit<1>)(tci >> 12);
                hdr.vlan.vid = (bit<12>)tci;
                hdr.vlan.ether_type = meta.kind;
                hdr.vlan.setValid();
            }
        } else if (meta.present == 0 && meta.mode == 3) {
            /* Nothing to move: ask the stack to tag the outgoing frame. */
            skb_vlan_push((bit<16>)0x8100, (bit<16>)0x0064);
        } else if (meta.mode == 5) {
            /* Pop the outermost tag.  A tag left in the frame comes out of
             * the bytes, and when the metadata tag goes, the tag behind it
             * moves from the frame into metadata. */
            skb_vlan_pop();
            if (hdr.inner.isValid()) {
                hdr.ethernet.kind = hdr.inner.ether_type;
                hdr.inner.setInvalid();
            }
        }
        send_to_port((PortId_t)1);
    }
}
control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta,
                 in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit(hdr.ethernet);
        pkt.emit(hdr.inner);
        pkt.emit(hdr.vlan);
        pkt.emit(hdr.mode);
    }
}
PNA_NIC(Parser(), Ingress(), Deparser()) main;

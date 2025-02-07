#include <tofino/constants.p4>
#include <tofino/intrinsic_metadata.p4>
#include <tofino/primitives.p4>

#define VLAN_DEPTH             2
#define ETHERTYPE_VLAN         0x8100
#define ETHERTYPE_IPV4         0x0800

header_type ethernet_t {
    fields {
        dstAddr : 48;
        srcAddr : 48;
        etherType : 16;
    }
}

header_type vlan_tag_t {
    fields {
        pcp : 3;
        cfi : 1;
        vid : 12;
        etherType : 16;
    }
}


header ethernet_t ethernet;
header vlan_tag_t vlan_tag_;

parser start {
    return parse_ethernet;
}

parser parse_ethernet {
    extract(ethernet);
    return select(latest.etherType) {
        ETHERTYPE_VLAN : parse_vlan;
        default : ingress;
    }
}

parser parse_vlan {
    extract(vlan_tag_);
    return ingress;
}


action dmac_miss(flood_mc_index) {
    modify_field(ig_intr_md_for_tm.mcast_grp_a, flood_mc_index);
}

action dmac_unicast_hit(egress_port) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
}

action dmac_multicast_hit(mc_index) {
    modify_field(ig_intr_md_for_tm.mcast_grp_a, mc_index);
    modify_field(ig_intr_md_for_tm.enable_mcast_cutthru, 1);
}

action dmac_uc_mc_hit(egress_port, mc_index) {
    modify_field(ig_intr_md_for_tm.ucast_egress_port, egress_port);
    modify_field(ig_intr_md_for_tm.mcast_grp_a, mc_index);
    modify_field(ig_intr_md_for_tm.enable_mcast_cutthru, 1);
}

table dmac {
    reads {
        ethernet.dstAddr : exact;
    }
    actions {
        dmac_miss;
        dmac_unicast_hit;
        dmac_multicast_hit;
        dmac_uc_mc_hit;
    }
    size : 4096;
}

action qos_miss() {
    modify_field(ig_intr_md_for_tm.ingress_cos, 0);
    modify_field(ig_intr_md_for_tm.qid, 0);
}

action qos_hit_eg_bypass_no_mirror(qid, color) {
    modify_field(ig_intr_md_for_tm.ingress_cos, 1);
    modify_field(ig_intr_md_for_tm.qid, qid);
    bypass_egress();
#ifndef SINGLE_STAGE
    modify_field(ig_intr_md_for_tm.packet_color, color);
#else
    modify_field(ig_intr_md_for_tm._pad5, color);
#endif /* !SINGLE_STAGE */
}

action qos_hit_no_eg_bypass_no_mirror(qid, color) {
    modify_field(ig_intr_md_for_tm.ingress_cos, 1);
    modify_field(ig_intr_md_for_tm.qid, qid);
#ifndef SINGLE_STAGE
    modify_field(ig_intr_md_for_tm.packet_color, color);
#else
    modify_field(ig_intr_md_for_tm._pad5, color);
#endif /* !SINGLE_STAGE */
}

action qos_hit_eg_bypass_i2e_mirror(qid, mirror_id) {
    modify_field(ig_intr_md_for_tm.ingress_cos, 1);
    modify_field(ig_intr_md_for_tm.qid, qid);
    bypass_egress();
    clone_ingress_pkt_to_egress(mirror_id);
}

action qos_hit_e2e_mirror(qid, mirror_id) {
    clone_egress_pkt_to_egress(mirror_id);
}


table ingress_qos {
    reads {
        vlan_tag_: valid;
        vlan_tag_.vid : exact;
    }
    actions {
        qos_miss;
        qos_hit_eg_bypass_i2e_mirror;
        qos_hit_eg_bypass_no_mirror;
        qos_hit_no_eg_bypass_no_mirror;
    }
    size : 8192;
}

action do_resubmit() {
    resubmit();
}

table resubmit_tbl {
    actions { do_resubmit; }
}

action do_deflect_on_drop() {
    modify_field(ig_intr_md_for_tm.deflect_on_drop, 1);
}

table deflect_on_drop_tbl {
    actions { do_deflect_on_drop; }
}

action do_recirc() {
    recirculate(68);
    invalidate(ig_intr_md_for_tm.mcast_grp_a);
}

action noop() {
    no_op();
}

table recirc_tbl {
    reads {
        ig_intr_md.ingress_port : exact;
    }
    actions {
        do_recirc;
        noop;
    }
}


table egress_qos {
    reads {
        vlan_tag_: valid;
        vlan_tag_.vid : exact;
    }
    actions {
        qos_hit_e2e_mirror;
    }
    size : 8192;
}



control ingress {
    apply(dmac);
    apply(ingress_qos);
#ifndef SINGLE_STAGE
    if (0 == ig_intr_md.resubmit_flag and
        1 == vlan_tag_.pcp) {
        apply(resubmit_tbl);
    } else
    if (2 == vlan_tag_.pcp) {
        apply(deflect_on_drop_tbl);
    } else
    if (3 == vlan_tag_.pcp) {
        apply(recirc_tbl);
    }
#endif /* !SINGLE_STAGE */
}

control egress {
    if (pkt_is_not_mirrored) {
         apply(egress_qos);
    }
}

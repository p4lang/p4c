#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

enum bit<8> FieldLists {
    none = 0,
    clone_e2e_FL = 1,
    recirculate_FL = 2,
    resubmit_FL = 3
}

struct intrinsic_metadata_t {
    bit<48> ingress_global_timestamp;
    bit<48> egress_global_timestamp;
    bit<16> mcast_grp;
    bit<16> egress_rid;
}

struct mymeta_t {
    @field_list(FieldLists.resubmit_FL) 
    bit<8> resubmit_count;
    @field_list(FieldLists.recirculate_FL) 
    bit<8> recirculate_count;
    @field_list(FieldLists.clone_e2e_FL) 
    bit<8> clone_e2e_count;
    bit<8> last_ing_instance_type;
    @field_list(FieldLists.clone_e2e_FL, FieldLists.recirculate_FL, FieldLists.resubmit_FL) 
    bit<8> f1;
}

struct temporaries_t {
    bit<48> temp1;
}

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct metadata {
    @name(".mymeta") 
    mymeta_t      mymeta;
    @name(".temporaries") 
    temporaries_t temporaries;
}

struct headers {
    @name(".ethernet") 
    ethernet_t ethernet;
}

parser ParserImpl(packet_in packet, out headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".parse_ethernet") state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".do_clone_e2e") action do_clone_e2e() {
        hdr.ethernet.srcAddr = hdr.ethernet.srcAddr - 48w23;
        meta.mymeta.f1 = meta.mymeta.f1 + 8w23;
        meta.mymeta.clone_e2e_count = meta.mymeta.clone_e2e_count + 8w1;
        clone_preserving_field_list(CloneType.E2E, (bit<32>)32w1, (bit<8>)FieldLists.clone_e2e_FL);
    }
    @name(".do_recirculate") action do_recirculate() {
        hdr.ethernet.srcAddr = hdr.ethernet.srcAddr - 48w19;
        meta.mymeta.f1 = meta.mymeta.f1 + 8w19;
        meta.mymeta.recirculate_count = meta.mymeta.recirculate_count + 8w1;
        recirculate_preserving_field_list((bit<8>)FieldLists.recirculate_FL);
    }
    @name("._nop") action _nop() {
    }
    @name(".put_debug_vals_in_eth_dstaddr") action put_debug_vals_in_eth_dstaddr() {
        hdr.ethernet.dstAddr = 48w0;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.resubmit_count << 40;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | (bit<48>)meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.recirculate_count << 32;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | (bit<48>)meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.clone_e2e_count << 24;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | (bit<48>)meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.f1 << 16;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | (bit<48>)meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.last_ing_instance_type << 8;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | (bit<48>)meta.temporaries.temp1;
    }
    @name(".mark_egr_resubmit_packet") action mark_egr_resubmit_packet() {
        put_debug_vals_in_eth_dstaddr();
    }
    @name(".mark_max_clone_e2e_packet") action mark_max_clone_e2e_packet() {
        put_debug_vals_in_eth_dstaddr();
        hdr.ethernet.etherType = 16w0xce2e;
    }
    @name(".mark_max_recirculate_packet") action mark_max_recirculate_packet() {
        put_debug_vals_in_eth_dstaddr();
        hdr.ethernet.etherType = 16w0xec14;
    }
    @name(".mark_vanilla_packet") action mark_vanilla_packet() {
        put_debug_vals_in_eth_dstaddr();
        hdr.ethernet.etherType = 16w0xf00f;
    }
    @name(".t_do_clone_e2e") table t_do_clone_e2e {
        actions = {
            do_clone_e2e;
        }
        key = {
        }
        default_action = do_clone_e2e();
    }
    @name(".t_do_recirculate") table t_do_recirculate {
        actions = {
            do_recirculate;
        }
        key = {
        }
        default_action = do_recirculate();
    }
    @name(".t_egr_debug_table1") table t_egr_debug_table1 {
        actions = {
            _nop;
        }
        key = {
            standard_metadata.ingress_port            : exact;
            standard_metadata.packet_length           : exact;
            standard_metadata.egress_spec             : exact;
            standard_metadata.egress_port             : exact;
            standard_metadata.instance_type           : exact;
            standard_metadata.ingress_global_timestamp: exact;
            standard_metadata.egress_global_timestamp : exact;
            standard_metadata.mcast_grp               : exact;
            standard_metadata.egress_rid              : exact;
            meta.mymeta.resubmit_count                : exact;
            meta.mymeta.recirculate_count             : exact;
            meta.mymeta.clone_e2e_count               : exact;
            meta.mymeta.f1                            : exact;
            meta.mymeta.last_ing_instance_type        : exact;
            hdr.ethernet.dstAddr                      : exact;
            hdr.ethernet.srcAddr                      : exact;
            hdr.ethernet.etherType                    : exact;
        }
        default_action = _nop();
    }
    @name(".t_egr_debug_table2") table t_egr_debug_table2 {
        actions = {
            _nop;
        }
        key = {
            standard_metadata.ingress_port            : exact;
            standard_metadata.packet_length           : exact;
            standard_metadata.egress_spec             : exact;
            standard_metadata.egress_port             : exact;
            standard_metadata.instance_type           : exact;
            standard_metadata.ingress_global_timestamp: exact;
            standard_metadata.egress_global_timestamp : exact;
            standard_metadata.mcast_grp               : exact;
            standard_metadata.egress_rid              : exact;
            meta.mymeta.resubmit_count                : exact;
            meta.mymeta.recirculate_count             : exact;
            meta.mymeta.clone_e2e_count               : exact;
            meta.mymeta.f1                            : exact;
            meta.mymeta.last_ing_instance_type        : exact;
            hdr.ethernet.dstAddr                      : exact;
            hdr.ethernet.srcAddr                      : exact;
            hdr.ethernet.etherType                    : exact;
        }
        default_action = _nop();
    }
    @name(".t_egr_mark_resubmit_packet") table t_egr_mark_resubmit_packet {
        actions = {
            mark_egr_resubmit_packet;
        }
        key = {
        }
        default_action = mark_egr_resubmit_packet();
    }
    @name(".t_mark_max_clone_e2e_packet") table t_mark_max_clone_e2e_packet {
        actions = {
            mark_max_clone_e2e_packet;
        }
        key = {
        }
        default_action = mark_max_clone_e2e_packet();
    }
    @name(".t_mark_max_recirculate_packet") table t_mark_max_recirculate_packet {
        actions = {
            mark_max_recirculate_packet;
        }
        key = {
        }
        default_action = mark_max_recirculate_packet();
    }
    @name(".t_mark_vanilla_packet") table t_mark_vanilla_packet {
        actions = {
            mark_vanilla_packet;
        }
        key = {
        }
        default_action = mark_vanilla_packet();
    }
    apply {
        t_egr_debug_table1.apply();
        if (hdr.ethernet.dstAddr == 48w0x1) {
            t_egr_mark_resubmit_packet.apply();
        } else {
            if (hdr.ethernet.dstAddr == 48w0x2) {
                if (meta.mymeta.recirculate_count < 8w5) {
                    t_do_recirculate.apply();
                } else {
                    t_mark_max_recirculate_packet.apply();
                }
            } else {
                if (hdr.ethernet.dstAddr == 48w0x3) {
                    if (meta.mymeta.clone_e2e_count < 8w4) {
                        t_do_clone_e2e.apply();
                    } else {
                        t_mark_max_clone_e2e_packet.apply();
                    }
                } else {
                    t_mark_vanilla_packet.apply();
                }
            }
        }
        t_egr_debug_table2.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".do_resubmit") action do_resubmit() {
        hdr.ethernet.srcAddr = hdr.ethernet.srcAddr - 48w17;
        meta.mymeta.f1 = meta.mymeta.f1 + 8w17;
        meta.mymeta.resubmit_count = meta.mymeta.resubmit_count + 8w1;
        resubmit_preserving_field_list((bit<8>)FieldLists.resubmit_FL);
    }
    @name("._nop") action _nop() {
    }
    @name(".set_port_to_mac_da_lsbs") action set_port_to_mac_da_lsbs() {
        standard_metadata.egress_spec = (bit<9>)hdr.ethernet.dstAddr & 9w0xf;
    }
    @name(".mark_max_resubmit_packet") action mark_max_resubmit_packet() {
        hdr.ethernet.etherType = 16w0xe50b;
    }
    @name(".save_ing_instance_type") action save_ing_instance_type() {
        meta.mymeta.last_ing_instance_type = (bit<8>)standard_metadata.instance_type;
    }
    @name(".t_do_resubmit") table t_do_resubmit {
        actions = {
            do_resubmit;
        }
        key = {
        }
        default_action = do_resubmit();
    }
    @name(".t_ing_debug_table1") table t_ing_debug_table1 {
        actions = {
            _nop;
        }
        key = {
            standard_metadata.ingress_port            : exact;
            standard_metadata.packet_length           : exact;
            standard_metadata.egress_spec             : exact;
            standard_metadata.egress_port             : exact;
            standard_metadata.instance_type           : exact;
            standard_metadata.ingress_global_timestamp: exact;
            standard_metadata.egress_global_timestamp : exact;
            standard_metadata.mcast_grp               : exact;
            standard_metadata.egress_rid              : exact;
            meta.mymeta.resubmit_count                : exact;
            meta.mymeta.recirculate_count             : exact;
            meta.mymeta.clone_e2e_count               : exact;
            meta.mymeta.f1                            : exact;
            meta.mymeta.last_ing_instance_type        : exact;
            hdr.ethernet.dstAddr                      : exact;
            hdr.ethernet.srcAddr                      : exact;
            hdr.ethernet.etherType                    : exact;
        }
        default_action = _nop();
    }
    @name(".t_ing_debug_table2") table t_ing_debug_table2 {
        actions = {
            _nop;
        }
        key = {
            standard_metadata.ingress_port            : exact;
            standard_metadata.packet_length           : exact;
            standard_metadata.egress_spec             : exact;
            standard_metadata.egress_port             : exact;
            standard_metadata.instance_type           : exact;
            standard_metadata.ingress_global_timestamp: exact;
            standard_metadata.egress_global_timestamp : exact;
            standard_metadata.mcast_grp               : exact;
            standard_metadata.egress_rid              : exact;
            meta.mymeta.resubmit_count                : exact;
            meta.mymeta.recirculate_count             : exact;
            meta.mymeta.clone_e2e_count               : exact;
            meta.mymeta.f1                            : exact;
            meta.mymeta.last_ing_instance_type        : exact;
            hdr.ethernet.dstAddr                      : exact;
            hdr.ethernet.srcAddr                      : exact;
            hdr.ethernet.etherType                    : exact;
        }
        default_action = _nop();
    }
    @name(".t_ing_mac_da") table t_ing_mac_da {
        actions = {
            set_port_to_mac_da_lsbs;
        }
        key = {
        }
        default_action = set_port_to_mac_da_lsbs();
    }
    @name(".t_mark_max_resubmit_packet") table t_mark_max_resubmit_packet {
        actions = {
            mark_max_resubmit_packet;
        }
        key = {
        }
        default_action = mark_max_resubmit_packet();
    }
    @name(".t_save_ing_instance_type") table t_save_ing_instance_type {
        actions = {
            save_ing_instance_type;
        }
        key = {
        }
        default_action = save_ing_instance_type();
    }
    apply {
        t_ing_debug_table1.apply();
        if (hdr.ethernet.dstAddr == 48w0x1) {
            if (meta.mymeta.resubmit_count < 8w3) {
                t_do_resubmit.apply();
            } else {
                t_mark_max_resubmit_packet.apply();
            }
        } else {
            t_ing_mac_da.apply();
        }
        t_save_ing_instance_type.apply();
        t_ing_debug_table2.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit(hdr.ethernet);
    }
}

control verifyChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

control computeChecksum(inout headers hdr, inout metadata meta) {
    apply {
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;


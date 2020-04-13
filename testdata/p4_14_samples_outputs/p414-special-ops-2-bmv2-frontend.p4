#include <core.p4>
#define V1MODEL_VERSION 20200408
#include <v1model.p4>

struct intrinsic_metadata_t {
    bit<48> ingress_global_timestamp;
    bit<48> egress_global_timestamp;
    bit<16> mcast_grp;
    bit<16> egress_rid;
}

struct mymeta_t {
    bit<8> resubmit_count;
    bit<8> recirculate_count;
    bit<8> clone_e2e_count;
    bit<8> last_ing_instance_type;
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
        packet.extract<ethernet_t>(hdr.ethernet);
        transition accept;
    }
    @name(".start") state start {
        transition parse_ethernet;
    }
}

control egress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".do_clone_e2e") action do_clone_e2e() {
        hdr.ethernet.srcAddr = hdr.ethernet.srcAddr + 48w281474976710633;
        meta.mymeta.f1 = meta.mymeta.f1 + 8w23;
        meta.mymeta.clone_e2e_count = meta.mymeta.clone_e2e_count + 8w1;
        clone3<tuple<mymeta_t>>(CloneType.E2E, 32w1, { meta.mymeta });
    }
    @name(".do_recirculate") action do_recirculate() {
        hdr.ethernet.srcAddr = hdr.ethernet.srcAddr + 48w281474976710637;
        meta.mymeta.f1 = meta.mymeta.f1 + 8w19;
        meta.mymeta.recirculate_count = meta.mymeta.recirculate_count + 8w1;
        recirculate<tuple<mymeta_t>>({ meta.mymeta });
    }
    @name("._nop") action _nop() {
    }
    @name("._nop") action _nop_2() {
    }
    @name(".mark_egr_resubmit_packet") action mark_egr_resubmit_packet() {
        hdr.ethernet.dstAddr = 48w0;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.resubmit_count << 40;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.recirculate_count << 32;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.clone_e2e_count << 24;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.f1 << 16;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.last_ing_instance_type << 8;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
    }
    @name(".mark_max_clone_e2e_packet") action mark_max_clone_e2e_packet() {
        hdr.ethernet.dstAddr = 48w0;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.resubmit_count << 40;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.recirculate_count << 32;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.clone_e2e_count << 24;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.f1 << 16;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.last_ing_instance_type << 8;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        hdr.ethernet.etherType = 16w0xce2e;
    }
    @name(".mark_max_recirculate_packet") action mark_max_recirculate_packet() {
        hdr.ethernet.dstAddr = 48w0;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.resubmit_count << 40;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.recirculate_count << 32;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.clone_e2e_count << 24;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.f1 << 16;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.last_ing_instance_type << 8;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        hdr.ethernet.etherType = 16w0xec14;
    }
    @name(".mark_vanilla_packet") action mark_vanilla_packet() {
        hdr.ethernet.dstAddr = 48w0;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.resubmit_count << 40;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.recirculate_count << 32;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.clone_e2e_count << 24;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.f1 << 16;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        meta.temporaries.temp1 = (bit<48>)meta.mymeta.last_ing_instance_type << 8;
        hdr.ethernet.dstAddr = hdr.ethernet.dstAddr | meta.temporaries.temp1;
        hdr.ethernet.etherType = 16w0xf00f;
    }
    @name(".t_do_clone_e2e") table t_do_clone_e2e_0 {
        actions = {
            do_clone_e2e();
        }
        key = {
        }
        default_action = do_clone_e2e();
    }
    @name(".t_do_recirculate") table t_do_recirculate_0 {
        actions = {
            do_recirculate();
        }
        key = {
        }
        default_action = do_recirculate();
    }
    @name(".t_egr_debug_table1") table t_egr_debug_table1_0 {
        actions = {
            _nop();
        }
        key = {
            standard_metadata.ingress_port            : exact @name("standard_metadata.ingress_port") ;
            standard_metadata.packet_length           : exact @name("standard_metadata.packet_length") ;
            standard_metadata.egress_spec             : exact @name("standard_metadata.egress_spec") ;
            standard_metadata.egress_port             : exact @name("standard_metadata.egress_port") ;
            standard_metadata.instance_type           : exact @name("standard_metadata.instance_type") ;
            standard_metadata.ingress_global_timestamp: exact @name("standard_metadata.ingress_global_timestamp") ;
            standard_metadata.egress_global_timestamp : exact @name("standard_metadata.egress_global_timestamp") ;
            standard_metadata.mcast_grp               : exact @name("standard_metadata.mcast_grp") ;
            standard_metadata.egress_rid              : exact @name("standard_metadata.egress_rid") ;
            meta.mymeta.resubmit_count                : exact @name("mymeta.resubmit_count") ;
            meta.mymeta.recirculate_count             : exact @name("mymeta.recirculate_count") ;
            meta.mymeta.clone_e2e_count               : exact @name("mymeta.clone_e2e_count") ;
            meta.mymeta.f1                            : exact @name("mymeta.f1") ;
            meta.mymeta.last_ing_instance_type        : exact @name("mymeta.last_ing_instance_type") ;
            hdr.ethernet.dstAddr                      : exact @name("ethernet.dstAddr") ;
            hdr.ethernet.srcAddr                      : exact @name("ethernet.srcAddr") ;
            hdr.ethernet.etherType                    : exact @name("ethernet.etherType") ;
        }
        default_action = _nop();
    }
    @name(".t_egr_debug_table2") table t_egr_debug_table2_0 {
        actions = {
            _nop_2();
        }
        key = {
            standard_metadata.ingress_port            : exact @name("standard_metadata.ingress_port") ;
            standard_metadata.packet_length           : exact @name("standard_metadata.packet_length") ;
            standard_metadata.egress_spec             : exact @name("standard_metadata.egress_spec") ;
            standard_metadata.egress_port             : exact @name("standard_metadata.egress_port") ;
            standard_metadata.instance_type           : exact @name("standard_metadata.instance_type") ;
            standard_metadata.ingress_global_timestamp: exact @name("standard_metadata.ingress_global_timestamp") ;
            standard_metadata.egress_global_timestamp : exact @name("standard_metadata.egress_global_timestamp") ;
            standard_metadata.mcast_grp               : exact @name("standard_metadata.mcast_grp") ;
            standard_metadata.egress_rid              : exact @name("standard_metadata.egress_rid") ;
            meta.mymeta.resubmit_count                : exact @name("mymeta.resubmit_count") ;
            meta.mymeta.recirculate_count             : exact @name("mymeta.recirculate_count") ;
            meta.mymeta.clone_e2e_count               : exact @name("mymeta.clone_e2e_count") ;
            meta.mymeta.f1                            : exact @name("mymeta.f1") ;
            meta.mymeta.last_ing_instance_type        : exact @name("mymeta.last_ing_instance_type") ;
            hdr.ethernet.dstAddr                      : exact @name("ethernet.dstAddr") ;
            hdr.ethernet.srcAddr                      : exact @name("ethernet.srcAddr") ;
            hdr.ethernet.etherType                    : exact @name("ethernet.etherType") ;
        }
        default_action = _nop_2();
    }
    @name(".t_egr_mark_resubmit_packet") table t_egr_mark_resubmit_packet_0 {
        actions = {
            mark_egr_resubmit_packet();
        }
        key = {
        }
        default_action = mark_egr_resubmit_packet();
    }
    @name(".t_mark_max_clone_e2e_packet") table t_mark_max_clone_e2e_packet_0 {
        actions = {
            mark_max_clone_e2e_packet();
        }
        key = {
        }
        default_action = mark_max_clone_e2e_packet();
    }
    @name(".t_mark_max_recirculate_packet") table t_mark_max_recirculate_packet_0 {
        actions = {
            mark_max_recirculate_packet();
        }
        key = {
        }
        default_action = mark_max_recirculate_packet();
    }
    @name(".t_mark_vanilla_packet") table t_mark_vanilla_packet_0 {
        actions = {
            mark_vanilla_packet();
        }
        key = {
        }
        default_action = mark_vanilla_packet();
    }
    apply {
        t_egr_debug_table1_0.apply();
        if (hdr.ethernet.dstAddr == 48w0x1) {
            t_egr_mark_resubmit_packet_0.apply();
        } else if (hdr.ethernet.dstAddr == 48w0x2) {
            if (meta.mymeta.recirculate_count < 8w5) {
                t_do_recirculate_0.apply();
            } else {
                t_mark_max_recirculate_packet_0.apply();
            }
        } else if (hdr.ethernet.dstAddr == 48w0x3) {
            if (meta.mymeta.clone_e2e_count < 8w4) {
                t_do_clone_e2e_0.apply();
            } else {
                t_mark_max_clone_e2e_packet_0.apply();
            }
        } else {
            t_mark_vanilla_packet_0.apply();
        }
        t_egr_debug_table2_0.apply();
    }
}

control ingress(inout headers hdr, inout metadata meta, inout standard_metadata_t standard_metadata) {
    @name(".do_resubmit") action do_resubmit() {
        hdr.ethernet.srcAddr = hdr.ethernet.srcAddr + 48w281474976710639;
        meta.mymeta.f1 = meta.mymeta.f1 + 8w17;
        meta.mymeta.resubmit_count = meta.mymeta.resubmit_count + 8w1;
        resubmit<tuple<mymeta_t>>({ meta.mymeta });
    }
    @name("._nop") action _nop_5() {
    }
    @name("._nop") action _nop_6() {
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
    @name(".t_do_resubmit") table t_do_resubmit_0 {
        actions = {
            do_resubmit();
        }
        key = {
        }
        default_action = do_resubmit();
    }
    @name(".t_ing_debug_table1") table t_ing_debug_table1_0 {
        actions = {
            _nop_5();
        }
        key = {
            standard_metadata.ingress_port            : exact @name("standard_metadata.ingress_port") ;
            standard_metadata.packet_length           : exact @name("standard_metadata.packet_length") ;
            standard_metadata.egress_spec             : exact @name("standard_metadata.egress_spec") ;
            standard_metadata.egress_port             : exact @name("standard_metadata.egress_port") ;
            standard_metadata.instance_type           : exact @name("standard_metadata.instance_type") ;
            standard_metadata.ingress_global_timestamp: exact @name("standard_metadata.ingress_global_timestamp") ;
            standard_metadata.egress_global_timestamp : exact @name("standard_metadata.egress_global_timestamp") ;
            standard_metadata.mcast_grp               : exact @name("standard_metadata.mcast_grp") ;
            standard_metadata.egress_rid              : exact @name("standard_metadata.egress_rid") ;
            meta.mymeta.resubmit_count                : exact @name("mymeta.resubmit_count") ;
            meta.mymeta.recirculate_count             : exact @name("mymeta.recirculate_count") ;
            meta.mymeta.clone_e2e_count               : exact @name("mymeta.clone_e2e_count") ;
            meta.mymeta.f1                            : exact @name("mymeta.f1") ;
            meta.mymeta.last_ing_instance_type        : exact @name("mymeta.last_ing_instance_type") ;
            hdr.ethernet.dstAddr                      : exact @name("ethernet.dstAddr") ;
            hdr.ethernet.srcAddr                      : exact @name("ethernet.srcAddr") ;
            hdr.ethernet.etherType                    : exact @name("ethernet.etherType") ;
        }
        default_action = _nop_5();
    }
    @name(".t_ing_debug_table2") table t_ing_debug_table2_0 {
        actions = {
            _nop_6();
        }
        key = {
            standard_metadata.ingress_port            : exact @name("standard_metadata.ingress_port") ;
            standard_metadata.packet_length           : exact @name("standard_metadata.packet_length") ;
            standard_metadata.egress_spec             : exact @name("standard_metadata.egress_spec") ;
            standard_metadata.egress_port             : exact @name("standard_metadata.egress_port") ;
            standard_metadata.instance_type           : exact @name("standard_metadata.instance_type") ;
            standard_metadata.ingress_global_timestamp: exact @name("standard_metadata.ingress_global_timestamp") ;
            standard_metadata.egress_global_timestamp : exact @name("standard_metadata.egress_global_timestamp") ;
            standard_metadata.mcast_grp               : exact @name("standard_metadata.mcast_grp") ;
            standard_metadata.egress_rid              : exact @name("standard_metadata.egress_rid") ;
            meta.mymeta.resubmit_count                : exact @name("mymeta.resubmit_count") ;
            meta.mymeta.recirculate_count             : exact @name("mymeta.recirculate_count") ;
            meta.mymeta.clone_e2e_count               : exact @name("mymeta.clone_e2e_count") ;
            meta.mymeta.f1                            : exact @name("mymeta.f1") ;
            meta.mymeta.last_ing_instance_type        : exact @name("mymeta.last_ing_instance_type") ;
            hdr.ethernet.dstAddr                      : exact @name("ethernet.dstAddr") ;
            hdr.ethernet.srcAddr                      : exact @name("ethernet.srcAddr") ;
            hdr.ethernet.etherType                    : exact @name("ethernet.etherType") ;
        }
        default_action = _nop_6();
    }
    @name(".t_ing_mac_da") table t_ing_mac_da_0 {
        actions = {
            set_port_to_mac_da_lsbs();
        }
        key = {
        }
        default_action = set_port_to_mac_da_lsbs();
    }
    @name(".t_mark_max_resubmit_packet") table t_mark_max_resubmit_packet_0 {
        actions = {
            mark_max_resubmit_packet();
        }
        key = {
        }
        default_action = mark_max_resubmit_packet();
    }
    @name(".t_save_ing_instance_type") table t_save_ing_instance_type_0 {
        actions = {
            save_ing_instance_type();
        }
        key = {
        }
        default_action = save_ing_instance_type();
    }
    apply {
        t_ing_debug_table1_0.apply();
        if (hdr.ethernet.dstAddr == 48w0x1) {
            if (meta.mymeta.resubmit_count < 8w3) {
                t_do_resubmit_0.apply();
            } else {
                t_mark_max_resubmit_packet_0.apply();
            }
        } else {
            t_ing_mac_da_0.apply();
        }
        t_save_ing_instance_type_0.apply();
        t_ing_debug_table2_0.apply();
    }
}

control DeparserImpl(packet_out packet, in headers hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
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

V1Switch<headers, metadata>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;


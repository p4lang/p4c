#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> totalLen;
    bit<16> identification;
    bit<3>  flags;
    bit<13> fragOffset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdrChecksum;
    bit<32> srcAddr;
    bit<32> dstAddr;
}

header switch_to_cpu_header_t {
    bit<32> word0;
    bit<32> word1;
}

struct fwd_meta_t {
    bit<32> l2ptr;
    bit<24> out_bd;
}

struct meta_t {
    bit<32> _fwd_l2ptr0;
    bit<24> _fwd_out_bd1;
}

struct headers_t {
    switch_to_cpu_header_t switch_to_cpu;
    ethernet_t             ethernet;
    ipv4_t                 ipv4;
}

parser ParserImpl(packet_in packet, out headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    state start {
        packet.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hdr.ipv4);
        transition accept;
    }
}

struct tuple_0 {
}

control ingress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    standard_metadata_t smeta;
    standard_metadata_t smeta_1;
    @name(".my_drop") action my_drop() {
        smeta.ingress_port = standard_metadata.ingress_port;
        smeta.egress_spec = standard_metadata.egress_spec;
        smeta.egress_port = standard_metadata.egress_port;
        smeta.instance_type = standard_metadata.instance_type;
        smeta.packet_length = standard_metadata.packet_length;
        smeta.enq_timestamp = standard_metadata.enq_timestamp;
        smeta.enq_qdepth = standard_metadata.enq_qdepth;
        smeta.deq_timedelta = standard_metadata.deq_timedelta;
        smeta.deq_qdepth = standard_metadata.deq_qdepth;
        smeta.ingress_global_timestamp = standard_metadata.ingress_global_timestamp;
        smeta.egress_global_timestamp = standard_metadata.egress_global_timestamp;
        smeta.mcast_grp = standard_metadata.mcast_grp;
        smeta.egress_rid = standard_metadata.egress_rid;
        smeta.checksum_error = standard_metadata.checksum_error;
        smeta.parser_error = standard_metadata.parser_error;
        smeta.priority = standard_metadata.priority;
        mark_to_drop(smeta);
        standard_metadata.ingress_port = smeta.ingress_port;
        standard_metadata.egress_spec = smeta.egress_spec;
        standard_metadata.egress_port = smeta.egress_port;
        standard_metadata.instance_type = smeta.instance_type;
        standard_metadata.packet_length = smeta.packet_length;
        standard_metadata.enq_timestamp = smeta.enq_timestamp;
        standard_metadata.enq_qdepth = smeta.enq_qdepth;
        standard_metadata.deq_timedelta = smeta.deq_timedelta;
        standard_metadata.deq_qdepth = smeta.deq_qdepth;
        standard_metadata.ingress_global_timestamp = smeta.ingress_global_timestamp;
        standard_metadata.egress_global_timestamp = smeta.egress_global_timestamp;
        standard_metadata.mcast_grp = smeta.mcast_grp;
        standard_metadata.egress_rid = smeta.egress_rid;
        standard_metadata.checksum_error = smeta.checksum_error;
        standard_metadata.parser_error = smeta.parser_error;
        standard_metadata.priority = smeta.priority;
    }
    @name(".my_drop") action my_drop_0() {
        smeta_1.ingress_port = standard_metadata.ingress_port;
        smeta_1.egress_spec = standard_metadata.egress_spec;
        smeta_1.egress_port = standard_metadata.egress_port;
        smeta_1.instance_type = standard_metadata.instance_type;
        smeta_1.packet_length = standard_metadata.packet_length;
        smeta_1.enq_timestamp = standard_metadata.enq_timestamp;
        smeta_1.enq_qdepth = standard_metadata.enq_qdepth;
        smeta_1.deq_timedelta = standard_metadata.deq_timedelta;
        smeta_1.deq_qdepth = standard_metadata.deq_qdepth;
        smeta_1.ingress_global_timestamp = standard_metadata.ingress_global_timestamp;
        smeta_1.egress_global_timestamp = standard_metadata.egress_global_timestamp;
        smeta_1.mcast_grp = standard_metadata.mcast_grp;
        smeta_1.egress_rid = standard_metadata.egress_rid;
        smeta_1.checksum_error = standard_metadata.checksum_error;
        smeta_1.parser_error = standard_metadata.parser_error;
        smeta_1.priority = standard_metadata.priority;
        mark_to_drop(smeta_1);
        standard_metadata.ingress_port = smeta_1.ingress_port;
        standard_metadata.egress_spec = smeta_1.egress_spec;
        standard_metadata.egress_port = smeta_1.egress_port;
        standard_metadata.instance_type = smeta_1.instance_type;
        standard_metadata.packet_length = smeta_1.packet_length;
        standard_metadata.enq_timestamp = smeta_1.enq_timestamp;
        standard_metadata.enq_qdepth = smeta_1.enq_qdepth;
        standard_metadata.deq_timedelta = smeta_1.deq_timedelta;
        standard_metadata.deq_qdepth = smeta_1.deq_qdepth;
        standard_metadata.ingress_global_timestamp = smeta_1.ingress_global_timestamp;
        standard_metadata.egress_global_timestamp = smeta_1.egress_global_timestamp;
        standard_metadata.mcast_grp = smeta_1.mcast_grp;
        standard_metadata.egress_rid = smeta_1.egress_rid;
        standard_metadata.checksum_error = smeta_1.checksum_error;
        standard_metadata.parser_error = smeta_1.parser_error;
        standard_metadata.priority = smeta_1.priority;
    }
    @name("ingress.set_l2ptr") action set_l2ptr(bit<32> l2ptr) {
        meta._fwd_l2ptr0 = l2ptr;
    }
    @name("ingress.set_mcast_grp") action set_mcast_grp(bit<16> mcast_grp) {
        standard_metadata.mcast_grp = mcast_grp;
    }
    @name("ingress.do_resubmit") action do_resubmit(bit<32> new_ipv4_dstAddr) {
        hdr.ipv4.dstAddr = new_ipv4_dstAddr;
        resubmit<tuple_0>({  });
    }
    @name("ingress.do_clone_i2e") action do_clone_i2e(bit<32> l2ptr) {
        clone3<tuple_0>(CloneType.I2E, 32w5, {  });
        meta._fwd_l2ptr0 = l2ptr;
    }
    @name("ingress.ipv4_da_lpm") table ipv4_da_lpm_0 {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr") ;
        }
        actions = {
            set_l2ptr();
            set_mcast_grp();
            do_resubmit();
            do_clone_i2e();
            my_drop();
        }
        default_action = my_drop();
    }
    @name("ingress.set_bd_dmac_intf") action set_bd_dmac_intf(bit<24> bd, bit<48> dmac, bit<9> intf) {
        meta._fwd_out_bd1 = bd;
        hdr.ethernet.dstAddr = dmac;
        standard_metadata.egress_spec = intf;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("ingress.mac_da") table mac_da_0 {
        key = {
            meta._fwd_l2ptr0: exact @name("meta.fwd.l2ptr") ;
        }
        actions = {
            set_bd_dmac_intf();
            my_drop_0();
        }
        default_action = my_drop_0();
    }
    @hidden action v1modelspecialopsbmv2l312() {
        hdr.ipv4.srcAddr = 32w184320258;
        meta._fwd_l2ptr0 = 32w0xe50b;
    }
    @hidden action v1modelspecialopsbmv2l315() {
        hdr.ipv4.srcAddr = 32w180835939;
        meta._fwd_l2ptr0 = 32w0xec1c;
    }
    @hidden table tbl_v1modelspecialopsbmv2l312 {
        actions = {
            v1modelspecialopsbmv2l312();
        }
        const default_action = v1modelspecialopsbmv2l312();
    }
    @hidden table tbl_v1modelspecialopsbmv2l315 {
        actions = {
            v1modelspecialopsbmv2l315();
        }
        const default_action = v1modelspecialopsbmv2l315();
    }
    apply {
        if (standard_metadata.instance_type == 32w6) {
            tbl_v1modelspecialopsbmv2l312.apply();
        } else if (standard_metadata.instance_type == 32w4) {
            tbl_v1modelspecialopsbmv2l315.apply();
        } else {
            ipv4_da_lpm_0.apply();
        }
        if (meta._fwd_l2ptr0 != 32w0) {
            mac_da_0.apply();
        }
    }
}

control egress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    standard_metadata_t smeta_2;
    @noWarn("unused") @name(".NoAction") action NoAction_0() {
    }
    @name(".my_drop") action my_drop_1() {
        smeta_2.ingress_port = standard_metadata.ingress_port;
        smeta_2.egress_spec = standard_metadata.egress_spec;
        smeta_2.egress_port = standard_metadata.egress_port;
        smeta_2.instance_type = standard_metadata.instance_type;
        smeta_2.packet_length = standard_metadata.packet_length;
        smeta_2.enq_timestamp = standard_metadata.enq_timestamp;
        smeta_2.enq_qdepth = standard_metadata.enq_qdepth;
        smeta_2.deq_timedelta = standard_metadata.deq_timedelta;
        smeta_2.deq_qdepth = standard_metadata.deq_qdepth;
        smeta_2.ingress_global_timestamp = standard_metadata.ingress_global_timestamp;
        smeta_2.egress_global_timestamp = standard_metadata.egress_global_timestamp;
        smeta_2.mcast_grp = standard_metadata.mcast_grp;
        smeta_2.egress_rid = standard_metadata.egress_rid;
        smeta_2.checksum_error = standard_metadata.checksum_error;
        smeta_2.parser_error = standard_metadata.parser_error;
        smeta_2.priority = standard_metadata.priority;
        mark_to_drop(smeta_2);
        standard_metadata.ingress_port = smeta_2.ingress_port;
        standard_metadata.egress_spec = smeta_2.egress_spec;
        standard_metadata.egress_port = smeta_2.egress_port;
        standard_metadata.instance_type = smeta_2.instance_type;
        standard_metadata.packet_length = smeta_2.packet_length;
        standard_metadata.enq_timestamp = smeta_2.enq_timestamp;
        standard_metadata.enq_qdepth = smeta_2.enq_qdepth;
        standard_metadata.deq_timedelta = smeta_2.deq_timedelta;
        standard_metadata.deq_qdepth = smeta_2.deq_qdepth;
        standard_metadata.ingress_global_timestamp = smeta_2.ingress_global_timestamp;
        standard_metadata.egress_global_timestamp = smeta_2.egress_global_timestamp;
        standard_metadata.mcast_grp = smeta_2.mcast_grp;
        standard_metadata.egress_rid = smeta_2.egress_rid;
        standard_metadata.checksum_error = smeta_2.checksum_error;
        standard_metadata.parser_error = smeta_2.parser_error;
        standard_metadata.priority = smeta_2.priority;
    }
    @name("egress.set_out_bd") action set_out_bd(bit<24> bd) {
        meta._fwd_out_bd1 = bd;
    }
    @name("egress.get_multicast_copy_out_bd") table get_multicast_copy_out_bd_0 {
        key = {
            standard_metadata.mcast_grp : exact @name("standard_metadata.mcast_grp") ;
            standard_metadata.egress_rid: exact @name("standard_metadata.egress_rid") ;
        }
        actions = {
            set_out_bd();
            @defaultonly NoAction_0();
        }
        default_action = NoAction_0();
    }
    @name("egress.rewrite_mac") action rewrite_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name("egress.do_recirculate") action do_recirculate(bit<32> new_ipv4_dstAddr) {
        hdr.ipv4.dstAddr = new_ipv4_dstAddr;
        recirculate<tuple_0>({  });
    }
    @name("egress.do_clone_e2e") action do_clone_e2e(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        clone3<tuple_0>(CloneType.E2E, 32w11, {  });
    }
    @name("egress.send_frame") table send_frame_0 {
        key = {
            meta._fwd_out_bd1: exact @name("meta.fwd.out_bd") ;
        }
        actions = {
            rewrite_mac();
            do_recirculate();
            do_clone_e2e();
            my_drop_1();
        }
        default_action = my_drop_1();
    }
    @hidden action v1modelspecialopsbmv2l385() {
        hdr.switch_to_cpu.setValid();
        hdr.switch_to_cpu.word0 = 32w0x12e012e;
        hdr.switch_to_cpu.word1 = 32w0x5a5a5a5a;
    }
    @hidden action v1modelspecialopsbmv2l391() {
        hdr.switch_to_cpu.setValid();
        hdr.switch_to_cpu.word0 = 32w0xe2e0e2e;
        hdr.switch_to_cpu.word1 = 32w0x5a5a5a5a;
    }
    @hidden table tbl_v1modelspecialopsbmv2l385 {
        actions = {
            v1modelspecialopsbmv2l385();
        }
        const default_action = v1modelspecialopsbmv2l385();
    }
    @hidden table tbl_v1modelspecialopsbmv2l391 {
        actions = {
            v1modelspecialopsbmv2l391();
        }
        const default_action = v1modelspecialopsbmv2l391();
    }
    apply {
        if (standard_metadata.instance_type == 32w1) {
            tbl_v1modelspecialopsbmv2l385.apply();
        } else if (standard_metadata.instance_type == 32w2) {
            tbl_v1modelspecialopsbmv2l391.apply();
        } else {
            if (standard_metadata.instance_type == 32w5) {
                get_multicast_copy_out_bd_0.apply();
            }
            send_frame_0.apply();
        }
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<switch_to_cpu_header_t>(hdr.switch_to_cpu);
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

struct tuple_1 {
    bit<4>  f0;
    bit<4>  f1;
    bit<8>  f2;
    bit<16> f3;
    bit<16> f4;
    bit<3>  f5;
    bit<13> f6;
    bit<8>  f7;
    bit<8>  f8;
    bit<32> f9;
    bit<32> f10;
}

control verifyChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        verify_checksum<tuple_1, bit<16>>(hdr.ipv4.isValid() && hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        update_checksum<tuple_1, bit<16>>(hdr.ipv4.isValid() && hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers_t, meta_t>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;


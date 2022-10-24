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

control ingress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    @name("ingress.smeta") standard_metadata_t smeta_0;
    @name("ingress.smeta") standard_metadata_t smeta_3;
    @name(".my_drop") action my_drop_2() {
        smeta_0.ingress_port = standard_metadata.ingress_port;
        smeta_0.egress_spec = standard_metadata.egress_spec;
        smeta_0.egress_port = standard_metadata.egress_port;
        smeta_0.instance_type = standard_metadata.instance_type;
        smeta_0.packet_length = standard_metadata.packet_length;
        smeta_0.enq_timestamp = standard_metadata.enq_timestamp;
        smeta_0.enq_qdepth = standard_metadata.enq_qdepth;
        smeta_0.deq_timedelta = standard_metadata.deq_timedelta;
        smeta_0.deq_qdepth = standard_metadata.deq_qdepth;
        smeta_0.ingress_global_timestamp = standard_metadata.ingress_global_timestamp;
        smeta_0.egress_global_timestamp = standard_metadata.egress_global_timestamp;
        smeta_0.mcast_grp = standard_metadata.mcast_grp;
        smeta_0.egress_rid = standard_metadata.egress_rid;
        smeta_0.checksum_error = standard_metadata.checksum_error;
        smeta_0.parser_error = standard_metadata.parser_error;
        smeta_0.priority = standard_metadata.priority;
        mark_to_drop(smeta_0);
        standard_metadata.ingress_port = smeta_0.ingress_port;
        standard_metadata.egress_spec = smeta_0.egress_spec;
        standard_metadata.egress_port = smeta_0.egress_port;
        standard_metadata.instance_type = smeta_0.instance_type;
        standard_metadata.packet_length = smeta_0.packet_length;
        standard_metadata.enq_timestamp = smeta_0.enq_timestamp;
        standard_metadata.enq_qdepth = smeta_0.enq_qdepth;
        standard_metadata.deq_timedelta = smeta_0.deq_timedelta;
        standard_metadata.deq_qdepth = smeta_0.deq_qdepth;
        standard_metadata.ingress_global_timestamp = smeta_0.ingress_global_timestamp;
        standard_metadata.egress_global_timestamp = smeta_0.egress_global_timestamp;
        standard_metadata.mcast_grp = smeta_0.mcast_grp;
        standard_metadata.egress_rid = smeta_0.egress_rid;
        standard_metadata.checksum_error = smeta_0.checksum_error;
        standard_metadata.parser_error = smeta_0.parser_error;
        standard_metadata.priority = smeta_0.priority;
    }
    @name(".my_drop") action my_drop_3() {
        smeta_3.ingress_port = standard_metadata.ingress_port;
        smeta_3.egress_spec = standard_metadata.egress_spec;
        smeta_3.egress_port = standard_metadata.egress_port;
        smeta_3.instance_type = standard_metadata.instance_type;
        smeta_3.packet_length = standard_metadata.packet_length;
        smeta_3.enq_timestamp = standard_metadata.enq_timestamp;
        smeta_3.enq_qdepth = standard_metadata.enq_qdepth;
        smeta_3.deq_timedelta = standard_metadata.deq_timedelta;
        smeta_3.deq_qdepth = standard_metadata.deq_qdepth;
        smeta_3.ingress_global_timestamp = standard_metadata.ingress_global_timestamp;
        smeta_3.egress_global_timestamp = standard_metadata.egress_global_timestamp;
        smeta_3.mcast_grp = standard_metadata.mcast_grp;
        smeta_3.egress_rid = standard_metadata.egress_rid;
        smeta_3.checksum_error = standard_metadata.checksum_error;
        smeta_3.parser_error = standard_metadata.parser_error;
        smeta_3.priority = standard_metadata.priority;
        mark_to_drop(smeta_3);
        standard_metadata.ingress_port = smeta_3.ingress_port;
        standard_metadata.egress_spec = smeta_3.egress_spec;
        standard_metadata.egress_port = smeta_3.egress_port;
        standard_metadata.instance_type = smeta_3.instance_type;
        standard_metadata.packet_length = smeta_3.packet_length;
        standard_metadata.enq_timestamp = smeta_3.enq_timestamp;
        standard_metadata.enq_qdepth = smeta_3.enq_qdepth;
        standard_metadata.deq_timedelta = smeta_3.deq_timedelta;
        standard_metadata.deq_qdepth = smeta_3.deq_qdepth;
        standard_metadata.ingress_global_timestamp = smeta_3.ingress_global_timestamp;
        standard_metadata.egress_global_timestamp = smeta_3.egress_global_timestamp;
        standard_metadata.mcast_grp = smeta_3.mcast_grp;
        standard_metadata.egress_rid = smeta_3.egress_rid;
        standard_metadata.checksum_error = smeta_3.checksum_error;
        standard_metadata.parser_error = smeta_3.parser_error;
        standard_metadata.priority = smeta_3.priority;
    }
    @name("ingress.set_l2ptr") action set_l2ptr(@name("l2ptr") bit<32> l2ptr_2) {
        meta._fwd_l2ptr0 = l2ptr_2;
    }
    @name("ingress.set_mcast_grp") action set_mcast_grp(@name("mcast_grp") bit<16> mcast_grp_1) {
        standard_metadata.mcast_grp = mcast_grp_1;
    }
    @name("ingress.do_resubmit") action do_resubmit(@name("new_ipv4_dstAddr") bit<32> new_ipv4_dstAddr) {
        hdr.ipv4.dstAddr = new_ipv4_dstAddr;
        resubmit_preserving_field_list(8w0);
    }
    @name("ingress.do_clone_i2e") action do_clone_i2e(@name("l2ptr") bit<32> l2ptr_3) {
        clone_preserving_field_list(CloneType.I2E, 32w5, 8w0);
        meta._fwd_l2ptr0 = l2ptr_3;
    }
    @name("ingress.ipv4_da_lpm") table ipv4_da_lpm_0 {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr");
        }
        actions = {
            set_l2ptr();
            set_mcast_grp();
            do_resubmit();
            do_clone_i2e();
            my_drop_2();
        }
        default_action = my_drop_2();
    }
    @name("ingress.set_bd_dmac_intf") action set_bd_dmac_intf(@name("bd") bit<24> bd, @name("dmac") bit<48> dmac, @name("intf") bit<9> intf) {
        meta._fwd_out_bd1 = bd;
        hdr.ethernet.dstAddr = dmac;
        standard_metadata.egress_spec = intf;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("ingress.mac_da") table mac_da_0 {
        key = {
            meta._fwd_l2ptr0: exact @name("meta.fwd.l2ptr");
        }
        actions = {
            set_bd_dmac_intf();
            my_drop_3();
        }
        default_action = my_drop_3();
    }
    @hidden action v1modelspecialopsbmv2l292() {
        hdr.ipv4.srcAddr = 32w184320258;
        meta._fwd_l2ptr0 = 32w0xe50b;
    }
    @hidden action v1modelspecialopsbmv2l295() {
        hdr.ipv4.srcAddr = 32w180835939;
        meta._fwd_l2ptr0 = 32w0xec1c;
    }
    @hidden table tbl_v1modelspecialopsbmv2l292 {
        actions = {
            v1modelspecialopsbmv2l292();
        }
        const default_action = v1modelspecialopsbmv2l292();
    }
    @hidden table tbl_v1modelspecialopsbmv2l295 {
        actions = {
            v1modelspecialopsbmv2l295();
        }
        const default_action = v1modelspecialopsbmv2l295();
    }
    apply {
        if (standard_metadata.instance_type == 32w6) {
            tbl_v1modelspecialopsbmv2l292.apply();
        } else if (standard_metadata.instance_type == 32w4) {
            tbl_v1modelspecialopsbmv2l295.apply();
        } else {
            ipv4_da_lpm_0.apply();
        }
        if (meta._fwd_l2ptr0 != 32w0) {
            mac_da_0.apply();
        }
    }
}

control egress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    @name("egress.smeta") standard_metadata_t smeta_4;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name(".my_drop") action my_drop_4() {
        smeta_4.ingress_port = standard_metadata.ingress_port;
        smeta_4.egress_spec = standard_metadata.egress_spec;
        smeta_4.egress_port = standard_metadata.egress_port;
        smeta_4.instance_type = standard_metadata.instance_type;
        smeta_4.packet_length = standard_metadata.packet_length;
        smeta_4.enq_timestamp = standard_metadata.enq_timestamp;
        smeta_4.enq_qdepth = standard_metadata.enq_qdepth;
        smeta_4.deq_timedelta = standard_metadata.deq_timedelta;
        smeta_4.deq_qdepth = standard_metadata.deq_qdepth;
        smeta_4.ingress_global_timestamp = standard_metadata.ingress_global_timestamp;
        smeta_4.egress_global_timestamp = standard_metadata.egress_global_timestamp;
        smeta_4.mcast_grp = standard_metadata.mcast_grp;
        smeta_4.egress_rid = standard_metadata.egress_rid;
        smeta_4.checksum_error = standard_metadata.checksum_error;
        smeta_4.parser_error = standard_metadata.parser_error;
        smeta_4.priority = standard_metadata.priority;
        mark_to_drop(smeta_4);
        standard_metadata.ingress_port = smeta_4.ingress_port;
        standard_metadata.egress_spec = smeta_4.egress_spec;
        standard_metadata.egress_port = smeta_4.egress_port;
        standard_metadata.instance_type = smeta_4.instance_type;
        standard_metadata.packet_length = smeta_4.packet_length;
        standard_metadata.enq_timestamp = smeta_4.enq_timestamp;
        standard_metadata.enq_qdepth = smeta_4.enq_qdepth;
        standard_metadata.deq_timedelta = smeta_4.deq_timedelta;
        standard_metadata.deq_qdepth = smeta_4.deq_qdepth;
        standard_metadata.ingress_global_timestamp = smeta_4.ingress_global_timestamp;
        standard_metadata.egress_global_timestamp = smeta_4.egress_global_timestamp;
        standard_metadata.mcast_grp = smeta_4.mcast_grp;
        standard_metadata.egress_rid = smeta_4.egress_rid;
        standard_metadata.checksum_error = smeta_4.checksum_error;
        standard_metadata.parser_error = smeta_4.parser_error;
        standard_metadata.priority = smeta_4.priority;
    }
    @name("egress.set_out_bd") action set_out_bd(@name("bd") bit<24> bd_2) {
        meta._fwd_out_bd1 = bd_2;
    }
    @name("egress.get_multicast_copy_out_bd") table get_multicast_copy_out_bd_0 {
        key = {
            standard_metadata.mcast_grp : exact @name("standard_metadata.mcast_grp");
            standard_metadata.egress_rid: exact @name("standard_metadata.egress_rid");
        }
        actions = {
            set_out_bd();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("egress.rewrite_mac") action rewrite_mac(@name("smac") bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name("egress.do_recirculate") action do_recirculate(@name("new_ipv4_dstAddr") bit<32> new_ipv4_dstAddr_2) {
        hdr.ipv4.dstAddr = new_ipv4_dstAddr_2;
        recirculate_preserving_field_list(8w0);
    }
    @name("egress.do_clone_e2e") action do_clone_e2e(@name("smac") bit<48> smac_2) {
        hdr.ethernet.srcAddr = smac_2;
        clone_preserving_field_list(CloneType.E2E, 32w11, 8w0);
    }
    @name("egress.send_frame") table send_frame_0 {
        key = {
            meta._fwd_out_bd1: exact @name("meta.fwd.out_bd");
        }
        actions = {
            rewrite_mac();
            do_recirculate();
            do_clone_e2e();
            my_drop_4();
        }
        default_action = my_drop_4();
    }
    @hidden action v1modelspecialopsbmv2l367() {
        hdr.switch_to_cpu.setValid();
        hdr.switch_to_cpu.word0 = 32w0x12e012e;
        hdr.switch_to_cpu.word1 = 32w0x5a5a5a5a;
    }
    @hidden action v1modelspecialopsbmv2l373() {
        hdr.switch_to_cpu.setValid();
        hdr.switch_to_cpu.word0 = 32w0xe2e0e2e;
        hdr.switch_to_cpu.word1 = 32w0x5a5a5a5a;
    }
    @hidden table tbl_v1modelspecialopsbmv2l367 {
        actions = {
            v1modelspecialopsbmv2l367();
        }
        const default_action = v1modelspecialopsbmv2l367();
    }
    @hidden table tbl_v1modelspecialopsbmv2l373 {
        actions = {
            v1modelspecialopsbmv2l373();
        }
        const default_action = v1modelspecialopsbmv2l373();
    }
    apply {
        if (standard_metadata.instance_type == 32w1) {
            tbl_v1modelspecialopsbmv2l367.apply();
        } else if (standard_metadata.instance_type == 32w2) {
            tbl_v1modelspecialopsbmv2l373.apply();
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

struct tuple_0 {
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
        verify_checksum<tuple_0, bit<16>>(hdr.ipv4.isValid() && hdr.ipv4.ihl == 4w5, (tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr}, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        update_checksum<tuple_0, bit<16>>(hdr.ipv4.isValid() && hdr.ipv4.ihl == 4w5, (tuple_0){f0 = hdr.ipv4.version,f1 = hdr.ipv4.ihl,f2 = hdr.ipv4.diffserv,f3 = hdr.ipv4.totalLen,f4 = hdr.ipv4.identification,f5 = hdr.ipv4.flags,f6 = hdr.ipv4.fragOffset,f7 = hdr.ipv4.ttl,f8 = hdr.ipv4.protocol,f9 = hdr.ipv4.srcAddr,f10 = hdr.ipv4.dstAddr}, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers_t, meta_t>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;

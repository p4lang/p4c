#include <core.p4>
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
    fwd_meta_t fwd;
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
    @name(".my_drop") action my_drop() {
        mark_to_drop();
    }
    @name(".my_drop") action my_drop_0() {
        mark_to_drop();
    }
    bit<32> ipv4_address_0;
    bit<8> byte0_0;
    bit<8> byte1_0;
    bit<8> byte2_0;
    bit<8> byte3_0;
    @name("ingress.set_l2ptr") action set_l2ptr(bit<32> l2ptr) {
        meta.fwd.l2ptr = l2ptr;
    }
    @name("ingress.set_mcast_grp") action set_mcast_grp(bit<16> mcast_grp) {
        standard_metadata.mcast_grp = mcast_grp;
    }
    @name("ingress.do_resubmit") action do_resubmit(bit<32> new_ipv4_dstAddr) {
        hdr.ipv4.dstAddr = new_ipv4_dstAddr;
        resubmit<tuple<>>({  });
    }
    @name("ingress.do_clone_i2e") action do_clone_i2e(bit<32> l2ptr) {
        clone3<tuple<>>(CloneType.I2E, 32w5, {  });
        meta.fwd.l2ptr = l2ptr;
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
        meta.fwd.out_bd = bd;
        hdr.ethernet.dstAddr = dmac;
        standard_metadata.egress_spec = intf;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("ingress.mac_da") table mac_da_0 {
        key = {
            meta.fwd.l2ptr: exact @name("meta.fwd.l2ptr") ;
        }
        actions = {
            set_bd_dmac_intf();
            my_drop_0();
        }
        default_action = my_drop_0();
    }
    apply {
        if (standard_metadata.instance_type == 32w6) {
            byte0_0 = 8w10;
            byte1_0 = 8w252;
            byte2_0 = 8w129;
            byte3_0 = 8w2;
            ipv4_address_0 = byte0_0 ++ byte1_0 ++ byte2_0 ++ byte3_0;
            hdr.ipv4.srcAddr = ipv4_address_0;
            meta.fwd.l2ptr = 32w0xe50b;
        }
        else 
            if (standard_metadata.instance_type == 32w4) {
                byte0_0 = 8w10;
                byte1_0 = 8w199;
                byte2_0 = 8w86;
                byte3_0 = 8w99;
                ipv4_address_0 = byte0_0 ++ byte1_0 ++ byte2_0 ++ byte3_0;
                hdr.ipv4.srcAddr = ipv4_address_0;
                meta.fwd.l2ptr = 32w0xec1c;
            }
            else 
                ipv4_da_lpm_0.apply();
        if (meta.fwd.l2ptr != 32w0) 
            mac_da_0.apply();
    }
}

control egress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    @name(".NoAction") action NoAction_0() {
    }
    @name(".my_drop") action my_drop_1() {
        mark_to_drop();
    }
    @name("egress.set_out_bd") action set_out_bd(bit<24> bd) {
        meta.fwd.out_bd = bd;
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
        recirculate<tuple<>>({  });
    }
    @name("egress.do_clone_e2e") action do_clone_e2e(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        clone3<tuple<>>(CloneType.E2E, 32w11, {  });
    }
    @name("egress.send_frame") table send_frame_0 {
        key = {
            meta.fwd.out_bd: exact @name("meta.fwd.out_bd") ;
        }
        actions = {
            rewrite_mac();
            do_recirculate();
            do_clone_e2e();
            my_drop_1();
        }
        default_action = my_drop_1();
    }
    apply {
        if (standard_metadata.instance_type == 32w1) {
            hdr.switch_to_cpu.setValid();
            hdr.switch_to_cpu.word0 = 32w0x12e012e;
            hdr.switch_to_cpu.word1 = 32w0x5a5a5a5a;
        }
        else 
            if (standard_metadata.instance_type == 32w2) {
                hdr.switch_to_cpu.setValid();
                hdr.switch_to_cpu.word0 = 32w0xe2e0e2e;
                hdr.switch_to_cpu.word1 = 32w0x5a5a5a5a;
            }
            else {
                if (standard_metadata.instance_type == 32w5) 
                    get_multicast_copy_out_bd_0.apply();
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

control verifyChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        verify_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(hdr.ipv4.isValid() && hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        update_checksum<tuple<bit<4>, bit<4>, bit<8>, bit<16>, bit<16>, bit<3>, bit<13>, bit<8>, bit<8>, bit<32>, bit<32>>, bit<16>>(hdr.ipv4.isValid() && hdr.ipv4.ihl == 4w5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch<headers_t, meta_t>(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;


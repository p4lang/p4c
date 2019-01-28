#include <core.p4>
#include <v1model.p4>

const bit<32> BMV2_V1MODEL_INSTANCE_TYPE_NORMAL = 0;
const bit<32> BMV2_V1MODEL_INSTANCE_TYPE_INGRESS_CLONE = 1;
const bit<32> BMV2_V1MODEL_INSTANCE_TYPE_EGRESS_CLONE = 2;
const bit<32> BMV2_V1MODEL_INSTANCE_TYPE_COALESCED = 3;
const bit<32> BMV2_V1MODEL_INSTANCE_TYPE_RECIRC = 4;
const bit<32> BMV2_V1MODEL_INSTANCE_TYPE_REPLICATION = 5;
const bit<32> BMV2_V1MODEL_INSTANCE_TYPE_RESUBMIT = 6;
const bit<32> I2E_CLONE_SESSION_ID = 5;
const bit<32> E2E_CLONE_SESSION_ID = 11;
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

action my_drop() {
    mark_to_drop();
}
parser ParserImpl(packet_in packet, out headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    const bit<16> ETHERTYPE_IPV4 = 16w0x800;
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.ethernet);
        transition select(hdr.ethernet.etherType) {
            ETHERTYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition accept;
    }
}

control fill_ipv4_address(out bit<32> ipv4_address, in bit<8> byte0, in bit<8> byte1, in bit<8> byte2, in bit<8> byte3) {
    apply {
        ipv4_address = byte0 ++ byte1 ++ byte2 ++ byte3;
    }
}

control ingress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    fill_ipv4_address() c_fill_ipv4_address;
    const bit<32> RESUBMITTED_PKT_L2PTR = 0xe50b;
    const bit<32> RECIRCULATED_PKT_L2PTR = 0xec1c;
    action set_l2ptr(bit<32> l2ptr) {
        meta.fwd.l2ptr = l2ptr;
    }
    action set_mcast_grp(bit<16> mcast_grp) {
        standard_metadata.mcast_grp = mcast_grp;
    }
    action do_resubmit(bit<32> new_ipv4_dstAddr) {
        hdr.ipv4.dstAddr = new_ipv4_dstAddr;
        resubmit({  });
    }
    action do_clone_i2e(bit<32> l2ptr) {
        clone3(CloneType.I2E, I2E_CLONE_SESSION_ID, {  });
        meta.fwd.l2ptr = l2ptr;
    }
    table ipv4_da_lpm {
        key = {
            hdr.ipv4.dstAddr: lpm;
        }
        actions = {
            set_l2ptr;
            set_mcast_grp;
            do_resubmit;
            do_clone_i2e;
            my_drop;
        }
        default_action = my_drop;
    }
    action set_bd_dmac_intf(bit<24> bd, bit<48> dmac, bit<9> intf) {
        meta.fwd.out_bd = bd;
        hdr.ethernet.dstAddr = dmac;
        standard_metadata.egress_spec = intf;
        hdr.ipv4.ttl = hdr.ipv4.ttl - 1;
    }
    table mac_da {
        key = {
            meta.fwd.l2ptr: exact;
        }
        actions = {
            set_bd_dmac_intf;
            my_drop;
        }
        default_action = my_drop;
    }
    apply {
        if (standard_metadata.instance_type == BMV2_V1MODEL_INSTANCE_TYPE_RESUBMIT) {
            c_fill_ipv4_address.apply(hdr.ipv4.srcAddr, 10, 252, 129, 2);
            meta.fwd.l2ptr = RESUBMITTED_PKT_L2PTR;
        }
        else 
            if (standard_metadata.instance_type == BMV2_V1MODEL_INSTANCE_TYPE_RECIRC) {
                c_fill_ipv4_address.apply(hdr.ipv4.srcAddr, 10, 199, 86, 99);
                meta.fwd.l2ptr = RECIRCULATED_PKT_L2PTR;
            }
            else {
                ipv4_da_lpm.apply();
            }
        if (meta.fwd.l2ptr != 0) {
            mac_da.apply();
        }
    }
}

control egress(inout headers_t hdr, inout meta_t meta, inout standard_metadata_t standard_metadata) {
    action set_out_bd(bit<24> bd) {
        meta.fwd.out_bd = bd;
    }
    table get_multicast_copy_out_bd {
        key = {
            standard_metadata.mcast_grp : exact;
            standard_metadata.egress_rid: exact;
        }
        actions = {
            set_out_bd;
        }
    }
    action rewrite_mac(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    action do_recirculate(bit<32> new_ipv4_dstAddr) {
        hdr.ipv4.dstAddr = new_ipv4_dstAddr;
        recirculate({  });
    }
    action do_clone_e2e(bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
        clone3(CloneType.E2E, E2E_CLONE_SESSION_ID, {  });
    }
    table send_frame {
        key = {
            meta.fwd.out_bd: exact;
        }
        actions = {
            rewrite_mac;
            do_recirculate;
            do_clone_e2e;
            my_drop;
        }
        default_action = my_drop;
    }
    apply {
        if (standard_metadata.instance_type == BMV2_V1MODEL_INSTANCE_TYPE_INGRESS_CLONE) {
            hdr.switch_to_cpu.setValid();
            hdr.switch_to_cpu.word0 = 0x12e012e;
            hdr.switch_to_cpu.word1 = 0x5a5a5a5a;
        }
        else 
            if (standard_metadata.instance_type == BMV2_V1MODEL_INSTANCE_TYPE_EGRESS_CLONE) {
                hdr.switch_to_cpu.setValid();
                hdr.switch_to_cpu.word0 = 0xe2e0e2e;
                hdr.switch_to_cpu.word1 = 0x5a5a5a5a;
            }
            else {
                if (standard_metadata.instance_type == BMV2_V1MODEL_INSTANCE_TYPE_REPLICATION) {
                    get_multicast_copy_out_bd.apply();
                }
                send_frame.apply();
            }
    }
}

control DeparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit(hdr.switch_to_cpu);
        packet.emit(hdr.ethernet);
        packet.emit(hdr.ipv4);
    }
}

control verifyChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        verify_checksum(hdr.ipv4.isValid() && hdr.ipv4.ihl == 5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

control computeChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {
        update_checksum(hdr.ipv4.isValid() && hdr.ipv4.ihl == 5, { hdr.ipv4.version, hdr.ipv4.ihl, hdr.ipv4.diffserv, hdr.ipv4.totalLen, hdr.ipv4.identification, hdr.ipv4.flags, hdr.ipv4.fragOffset, hdr.ipv4.ttl, hdr.ipv4.protocol, hdr.ipv4.srcAddr, hdr.ipv4.dstAddr }, hdr.ipv4.hdrChecksum, HashAlgorithm.csum16);
    }
}

V1Switch(ParserImpl(), verifyChecksum(), ingress(), egress(), computeChecksum(), DeparserImpl()) main;


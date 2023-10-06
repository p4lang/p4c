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
    bit<8>  priority;
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

struct fwd_metadata_t {
    bit<32> l2ptr;
    bit<24> out_bd;
}

struct metadata_t {
    bit<32> _fwd_metadata_l2ptr0;
    bit<24> _fwd_metadata_out_bd1;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

parser parserImpl(packet_in packet, out headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
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

control ingressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("ingressImpl.prio") bit<8> prio_0;
    @name("ingressImpl.my_drop") action my_drop() {
        mark_to_drop(stdmeta);
    }
    @name("ingressImpl.my_drop") action my_drop_2() {
        mark_to_drop(stdmeta);
    }
    @name("ingressImpl.set_l2ptr_and_prio") action set_l2ptr_and_prio(@name("l2ptr") bit<32> l2ptr_1, @name("priority") bit<8> priority_1) {
        meta._fwd_metadata_l2ptr0 = l2ptr_1;
        prio_0 = priority_1;
    }
    @name("ingressImpl.ipv4_da_lpm") table ipv4_da_lpm_0 {
        key = {
            hdr.ipv4.dstAddr: lpm @name("hdr.ipv4.dstAddr");
        }
        actions = {
            set_l2ptr_and_prio();
            my_drop();
        }
        default_action = my_drop();
    }
    @name("ingressImpl.set_bd_dmac_intf") action set_bd_dmac_intf(@name("bd") bit<24> bd, @name("dmac") bit<48> dmac, @name("intf") bit<9> intf) {
        meta._fwd_metadata_out_bd1 = bd;
        hdr.ethernet.dstAddr = dmac;
        stdmeta.egress_spec = intf;
        hdr.ipv4.ttl = hdr.ipv4.ttl + 8w255;
    }
    @name("ingressImpl.mac_da") table mac_da_0 {
        key = {
            meta._fwd_metadata_l2ptr0: exact @name("meta.fwd_metadata.l2ptr");
        }
        actions = {
            set_bd_dmac_intf();
            my_drop_2();
        }
        default_action = my_drop_2();
    }
    @hidden action usepriorityasname132() {
        prio_0 = hdr.ipv4.priority;
    }
    @hidden action usepriorityasname84() {
        hdr.ipv4.priority = prio_0;
    }
    @hidden table tbl_usepriorityasname132 {
        actions = {
            usepriorityasname132();
        }
        const default_action = usepriorityasname132();
    }
    @hidden table tbl_usepriorityasname84 {
        actions = {
            usepriorityasname84();
        }
        const default_action = usepriorityasname84();
    }
    apply {
        if (hdr.ipv4.isValid()) {
            tbl_usepriorityasname132.apply();
            ipv4_da_lpm_0.apply();
            mac_da_0.apply();
            tbl_usepriorityasname84.apply();
        }
    }
}

control egressImpl(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t stdmeta) {
    @name("egressImpl.my_drop") action my_drop_3() {
        mark_to_drop(stdmeta);
    }
    @name("egressImpl.rewrite_mac") action rewrite_mac(@name("smac") bit<48> smac) {
        hdr.ethernet.srcAddr = smac;
    }
    @name("egressImpl.send_frame") table send_frame_0 {
        key = {
            meta._fwd_metadata_out_bd1: exact @name("meta.fwd_metadata.out_bd");
        }
        actions = {
            rewrite_mac();
            my_drop_3();
        }
        default_action = my_drop_3();
    }
    apply {
        send_frame_0.apply();
    }
}

control deparserImpl(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

control verifyChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control updateChecksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(parserImpl(), verifyChecksum(), ingressImpl(), egressImpl(), updateChecksum(), deparserImpl()) main;

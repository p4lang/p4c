#include <core.p4>
#define V1MODEL_VERSION 20180101
#include <v1model.p4>

header ethernet_t {
    bit<48> dst_addr;
    bit<48> src_addr;
    bit<16> ether_type;
}

header ipv4_t {
    bit<4>  version;
    bit<4>  ihl;
    bit<8>  diffserv;
    bit<16> total_len;
    bit<16> identification;
    bit<3>  flags;
    bit<13> frag_offset;
    bit<8>  ttl;
    bit<8>  protocol;
    bit<16> hdr_checksum;
    bit<32> src_addr;
    bit<32> dst_addr;
}

struct headers_t {
    ethernet_t ethernet;
    ipv4_t     ipv4;
}

struct metadata_t {
    bool    dropped;
    bit<16> direction;
    bit<32> dst_ip_addr;
    bit<32> src_ip_addr;
    bit<48> ethernet_addr;
}

parser test_parser(packet_in packet, out headers_t hd, inout metadata_t meta, inout standard_metadata_t standard_meta) {
    state start {
        packet.extract<ethernet_t>(hd.ethernet);
        transition select(hd.ethernet.ether_type) {
            16w0x800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract<ipv4_t>(hd.ipv4);
        transition accept;
    }
}

control test_deparser(packet_out packet, in headers_t hdr) {
    apply {
        packet.emit<ethernet_t>(hdr.ethernet);
        packet.emit<ipv4_t>(hdr.ipv4);
    }
}

control test_verify_checksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control test_compute_checksum(inout headers_t hdr, inout metadata_t meta) {
    apply {
    }
}

control test_ingress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_3() {
    }
    @name("test_ingress.drop_action") action drop_action() {
        mark_to_drop(standard_metadata);
    }
    @name("test_ingress.action3") action action3() {
        meta.dropped = true;
    }
    @name("test_ingress.action2") action action2() {
        meta.dst_ip_addr = hdr.ipv4.dst_addr;
    }
    @name("test_ingress.action1") action action1() {
        meta.src_ip_addr = hdr.ipv4.dst_addr;
    }
    @name("test_ingress.pre_tbl1") table pre_tbl1_0 {
        key = {
            hdr.ipv4.dst_addr: exact @name("hdr.ipv4.dst_addr");
        }
        actions = {
            action1();
            action2();
            action3();
            @defaultonly NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("test_ingress.action4") action action4() {
        meta.direction = 16w1;
    }
    @name("test_ingress.in_tbl2") table in_tbl2_0 {
        key = {
            hdr.ipv4.protocol: exact @name("hdr.ipv4.protocol");
        }
        actions = {
            action4();
            @defaultonly NoAction_2();
        }
        default_action = NoAction_2();
    }
    @name("test_ingress.action5") action action5(@name("neighbor_mac") bit<48> neighbor_mac) {
        meta.ethernet_addr = neighbor_mac;
    }
    @name("test_ingress.post_tbl3") table post_tbl3_0 {
        key = {
            hdr.ipv4.src_addr: exact @name("hdr.ipv4.src_addr");
        }
        actions = {
            action5();
            @defaultonly NoAction_3();
        }
        default_action = NoAction_3();
    }
    @hidden action issue3488bmv2l155() {
        standard_metadata.egress_spec = standard_metadata.ingress_port;
    }
    @hidden table tbl_issue3488bmv2l155 {
        actions = {
            issue3488bmv2l155();
        }
        const default_action = issue3488bmv2l155();
    }
    @hidden table tbl_drop_action {
        actions = {
            drop_action();
        }
        const default_action = drop_action();
    }
    apply {
        tbl_issue3488bmv2l155.apply();
        switch (pre_tbl1_0.apply().action_run) {
            action1: 
            action2: {
                in_tbl2_0.apply();
            }
            default: {
            }
        }
        post_tbl3_0.apply();
        if (meta.dropped) {
            tbl_drop_action.apply();
        }
    }
}

control test_egress(inout headers_t hdr, inout metadata_t meta, inout standard_metadata_t standard_metadata) {
    apply {
    }
}

V1Switch<headers_t, metadata_t>(test_parser(), test_verify_checksum(), test_ingress(), test_egress(), test_compute_checksum(), test_deparser()) main;

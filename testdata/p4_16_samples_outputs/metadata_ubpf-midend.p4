#include <core.p4>
#include <ubpf_model.p4>

header Ethernet_h {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

struct Headers_t {
    Ethernet_h ethernet;
}

struct metadata {
    bit<16> etherType;
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract<Ethernet_h>(headers.ethernet);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @noWarn("unused") @name(".NoAction") action NoAction_2() {
    }
    @name("pipe.fill_metadata") action fill_metadata() {
        meta.etherType = headers.ethernet.etherType;
    }
    @name("pipe.tbl") table tbl_0 {
        key = {
            headers.ethernet.etherType: exact @name("headers.ethernet.etherType");
        }
        actions = {
            fill_metadata();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @name("pipe.change_etherType") action change_etherType() {
        headers.ethernet.etherType = 16w0x86dd;
    }
    @name("pipe.meta_based_tbl") table meta_based_tbl_0 {
        key = {
            meta.etherType: exact @name("meta.etherType");
        }
        actions = {
            change_etherType();
            NoAction_2();
        }
        default_action = NoAction_2();
    }
    apply {
        tbl_0.apply();
        meta_based_tbl_0.apply();
    }
}

control DeparserImpl(packet_out packet, in Headers_t headers) {
    @hidden action metadata_ubpf87() {
        packet.emit<Ethernet_h>(headers.ethernet);
    }
    @hidden table tbl_metadata_ubpf87 {
        actions = {
            metadata_ubpf87();
        }
        const default_action = metadata_ubpf87();
    }
    apply {
        tbl_metadata_ubpf87.apply();
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), DeparserImpl()) main;

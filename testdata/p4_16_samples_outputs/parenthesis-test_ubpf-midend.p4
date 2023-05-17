#include <core.p4>
#include <ubpf_model.p4>

header Ethernet_h {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header IPv4_h {
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

struct Headers_t {
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

struct metadata {
    bit<8> qfi;
    bit<8> filt_dir;
    bit<8> reflec_qos;
}

parser prs(packet_in p, out Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    state start {
        p.extract<Ethernet_h>(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800: ipv4;
            default: reject;
        }
    }
    state ipv4 {
        p.extract<IPv4_h>(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("pipe.Reject") action Reject() {
        mark_to_drop();
    }
    @name("pipe.filter_tbl") table filter_tbl_0 {
        key = {
            headers.ipv4.srcAddr: exact @name("headers.ipv4.srcAddr");
        }
        actions = {
            Reject();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action parenthesistest_ubpf95() {
        meta.qfi = 8w3;
    }
    @hidden table tbl_parenthesistest_ubpf95 {
        actions = {
            parenthesistest_ubpf95();
        }
        const default_action = parenthesistest_ubpf95();
    }
    apply {
        if (headers.ipv4.isValid()) {
            filter_tbl_0.apply();
        }
        if (meta.qfi != 8w0 && (meta.filt_dir == 8w2 || meta.reflec_qos == 8w1)) {
            tbl_parenthesistest_ubpf95.apply();
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    @hidden action parenthesistest_ubpf101() {
        packet.emit<Ethernet_h>(headers.ethernet);
        packet.emit<IPv4_h>(headers.ipv4);
    }
    @hidden table tbl_parenthesistest_ubpf101 {
        actions = {
            parenthesistest_ubpf101();
        }
        const default_action = parenthesistest_ubpf101();
    }
    apply {
        tbl_parenthesistest_ubpf101.apply();
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;

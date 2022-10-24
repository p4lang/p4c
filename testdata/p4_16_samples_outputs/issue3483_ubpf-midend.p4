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

struct Meta {
    bit<1> b;
}

parser prs(packet_in p, out Headers_t headers, inout Meta meta, inout standard_metadata std_meta) {
    state start {
        p.extract<Ethernet_h>(headers.ethernet);
        transition select(headers.ethernet.etherType) {
            16w0x800: ip;
            default: reject;
        }
    }
    state ip {
        p.extract<IPv4_h>(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout Meta meta, inout standard_metadata unused) {
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("pipe.Reject") action Reject(@name("add") bit<32> add) {
        headers.ipv4.srcAddr = add;
    }
    @name("pipe.Check_src_ip") table Check_src_ip_0 {
        key = {
            headers.ipv4.srcAddr: exact @name("headers.ipv4.srcAddr");
        }
        actions = {
            Reject();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    @hidden action issue3483_ubpf54() {
        meta.b = 1w1;
    }
    @hidden action issue3483_ubpf59() {
        meta.b = 1w0;
    }
    @hidden table tbl_issue3483_ubpf54 {
        actions = {
            issue3483_ubpf54();
        }
        const default_action = issue3483_ubpf54();
    }
    @hidden table tbl_issue3483_ubpf59 {
        actions = {
            issue3483_ubpf59();
        }
        const default_action = issue3483_ubpf59();
    }
    apply {
        if (Check_src_ip_0.apply().hit) {
            ;
        } else {
            tbl_issue3483_ubpf54.apply();
        }
        switch (Check_src_ip_0.apply().action_run) {
            Reject: {
                tbl_issue3483_ubpf59.apply();
            }
            NoAction_1: {
            }
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    @hidden action issue3483_ubpf68() {
        packet.emit<Ethernet_h>(headers.ethernet);
        packet.emit<IPv4_h>(headers.ipv4);
    }
    @hidden table tbl_issue3483_ubpf68 {
        actions = {
            issue3483_ubpf68();
        }
        const default_action = issue3483_ubpf68();
    }
    apply {
        tbl_issue3483_ubpf68.apply();
    }
}

ubpf<Headers_t, Meta>(prs(), pipe(), dprs()) main;

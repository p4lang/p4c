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
    @name("pipe.set_ipv4_version") action set_ipv4_version(@name("version") bit<4> version_1) {
        headers.ipv4.version = version_1;
    }
    @name("pipe.set_ihl") action set_ihl(@name("ihl") bit<4> ihl_2) {
        headers.ipv4.ihl = ihl_2;
    }
    @name("pipe.set_diffserv") action set_diffserv(@name("diffserv") bit<8> diffserv_2) {
        headers.ipv4.diffserv = diffserv_2;
    }
    @name("pipe.set_identification") action set_identification(@name("identification") bit<16> identification_1) {
        headers.ipv4.identification = identification_1;
    }
    @name("pipe.set_flags") action set_flags(@name("flags") bit<3> flags_3) {
        headers.ipv4.flags = flags_3;
    }
    @name("pipe.set_fragOffset") action set_fragOffset(@name("fragOffset") bit<13> fragOffset_3) {
        headers.ipv4.fragOffset = fragOffset_3;
    }
    @name("pipe.set_ttl") action set_ttl(@name("ttl") bit<8> ttl_2) {
        headers.ipv4.ttl = ttl_2;
    }
    @name("pipe.set_protocol") action set_protocol(@name("protocol") bit<8> protocol_1) {
        headers.ipv4.protocol = protocol_1;
    }
    @name("pipe.set_srcAddr") action set_srcAddr(@name("srcAddr") bit<32> srcAddr_3) {
        headers.ipv4.srcAddr = srcAddr_3;
    }
    @name("pipe.set_dstAddr") action set_dstAddr(@name("dstAddr") bit<32> dstAddr_2) {
        headers.ipv4.dstAddr = dstAddr_2;
    }
    @name("pipe.set_srcAddr_dstAddr") action set_srcAddr_dstAddr(@name("srcAddr") bit<32> srcAddr_4, @name("dstAddr") bit<32> dstAddr_3) {
        headers.ipv4.srcAddr = srcAddr_4;
        headers.ipv4.dstAddr = dstAddr_3;
    }
    @name("pipe.set_ihl_diffserv") action set_ihl_diffserv(@name("ihl") bit<4> ihl_3, @name("diffserv") bit<8> diffserv_3) {
        headers.ipv4.ihl = ihl_3;
        headers.ipv4.diffserv = diffserv_3;
    }
    @name("pipe.set_fragOffset_flags") action set_fragOffset_flags(@name("fragOffset") bit<13> fragOffset_4, @name("flags") bit<3> flags_4) {
        headers.ipv4.flags = flags_4;
        headers.ipv4.fragOffset = fragOffset_4;
    }
    @name("pipe.set_flags_ttl") action set_flags_ttl(@name("flags") bit<3> flags_5, @name("ttl") bit<8> ttl_3) {
        headers.ipv4.flags = flags_5;
        headers.ipv4.ttl = ttl_3;
    }
    @name("pipe.set_fragOffset_srcAddr") action set_fragOffset_srcAddr(@name("fragOffset") bit<13> fragOffset_5, @name("srcAddr") bit<32> srcAddr_5) {
        headers.ipv4.fragOffset = fragOffset_5;
        headers.ipv4.srcAddr = srcAddr_5;
    }
    @name("pipe.filter_tbl") table filter_tbl_0 {
        key = {
            headers.ipv4.srcAddr: exact @name("headers.ipv4.srcAddr");
        }
        actions = {
            set_ipv4_version();
            set_ihl();
            set_diffserv();
            set_identification();
            set_flags();
            set_fragOffset();
            set_ttl();
            set_protocol();
            set_srcAddr();
            set_dstAddr();
            set_srcAddr_dstAddr();
            set_ihl_diffserv();
            set_fragOffset_flags();
            set_flags_ttl();
            set_fragOffset_srcAddr();
            Reject();
            NoAction_1();
        }
        default_action = NoAction_1();
    }
    apply {
        filter_tbl_0.apply();
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    @hidden action ipv4actions_ubpf171() {
        packet.emit<Ethernet_h>(headers.ethernet);
        packet.emit<IPv4_h>(headers.ipv4);
    }
    @hidden table tbl_ipv4actions_ubpf171 {
        actions = {
            ipv4actions_ubpf171();
        }
        const default_action = ipv4actions_ubpf171();
    }
    apply {
        tbl_ipv4actions_ubpf171.apply();
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;

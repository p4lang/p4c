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
            16w0x800: ip;
            default: reject;
        }
    }
    state ip {
        p.extract<IPv4_h>(headers.ipv4);
        transition accept;
    }
}

control pipe(inout Headers_t headers, inout metadata meta, inout standard_metadata std_meta) {
    @name("pipe.hasReturned") bool hasReturned;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("pipe.Reject") action Reject(@name("add") bit<32> add) {
        mark_to_drop();
        headers.ipv4.srcAddr = add;
    }
    @name("pipe.Check_src_ip") table Check_src_ip_0 {
        key = {
            headers.ipv4.srcAddr : lpm @name("headers.ipv4.srcAddr");
            headers.ipv4.protocol: exact @name("headers.ipv4.protocol");
        }
        actions = {
            Reject();
            NoAction_1();
        }
        default_action = Reject(32w0);
    }
    @hidden action lpm_ubpf59() {
        headers.ipv4.setInvalid();
        headers.ipv4.setValid();
        mark_to_drop();
        hasReturned = true;
    }
    @hidden action act() {
        hasReturned = false;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_lpm_ubpf59 {
        actions = {
            lpm_ubpf59();
        }
        const default_action = lpm_ubpf59();
    }
    apply {
        tbl_act.apply();
        if (headers.ipv4.isValid()) {
            ;
        } else {
            tbl_lpm_ubpf59.apply();
        }
        if (hasReturned) {
            ;
        } else {
            Check_src_ip_0.apply();
        }
    }
}

control dprs(packet_out packet, in Headers_t headers) {
    @hidden action lpm_ubpf71() {
        packet.emit<Ethernet_h>(headers.ethernet);
        packet.emit<IPv4_h>(headers.ipv4);
    }
    @hidden table tbl_lpm_ubpf71 {
        actions = {
            lpm_ubpf71();
        }
        const default_action = lpm_ubpf71();
    }
    apply {
        tbl_lpm_ubpf71.apply();
    }
}

ubpf<Headers_t, metadata>(prs(), pipe(), dprs()) main;

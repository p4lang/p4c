#include <core.p4>
#include <ebpf_model.p4>

@ethernetaddress typedef bit<48> EthernetAddress;
@ipv4address typedef bit<32> IPv4Address;
header Ethernet_h {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header IPv4_h {
    bit<4>      version;
    bit<4>      ihl;
    bit<8>      diffserv;
    bit<16>     totalLen;
    bit<16>     identification;
    bit<3>      flags;
    bit<13>     fragOffset;
    bit<8>      ttl;
    bit<8>      protocol;
    bit<16>     hdrChecksum;
    IPv4Address srcAddr;
    IPv4Address dstAddr;
}

struct Headers_t {
    Ethernet_h ethernet;
    IPv4_h     ipv4;
}

parser prs(packet_in p, out Headers_t headers) {
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

control pipe(inout Headers_t headers, out bool pass) {
    @name(".NoAction") action NoAction_0() {
    }
    @name("pipe.Reject") action Reject_0(IPv4Address add) {
        pass = false;
        headers.ipv4.srcAddr = add;
    }
    @name("pipe.Check_src_ip") table Check_src_ip {
        key = {
            headers.ipv4.srcAddr: exact @name("headers.ipv4.srcAddr") ;
        }
        actions = {
            Reject_0();
            NoAction_0();
        }
        implementation = hash_table(32w1024);
        const default_action = NoAction_0();
    }
    @hidden action act() {
        pass = false;
    }
    @hidden action act_0() {
        pass = true;
    }
    @hidden table tbl_act {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    @hidden table tbl_act_0 {
        actions = {
            act();
        }
        const default_action = act();
    }
    apply {
        tbl_act.apply();
        switch (Check_src_ip.apply().action_run) {
            Reject_0: {
                tbl_act_0.apply();
            }
            NoAction_0: {
            }
        }

    }
}

ebpfFilter<Headers_t>(prs(), pipe()) main;


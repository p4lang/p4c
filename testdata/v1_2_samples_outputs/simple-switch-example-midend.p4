#include "/home/mbudiu/barefoot/git/p4c/build/../p4include/core.p4"
#include "../testdata/v1_2_samples/simple_model.p4"

typedef bit<48> @ethernetaddress EthernetAddress;
typedef bit<32> @ipv4address IPv4Address;
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

error {
    IPv4IncorrectVersion,
    IPv4OptionsNotSupported,
    IPv4ChecksumError
}

struct Parsed_packet {
    Ethernet_h ethernet;
    IPv4_h     ip;
}

parser TopParser(packet_in b, out Parsed_packet p) {
    Checksum16() ck;
    state start {
        b.extract(p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x800: parse_ipv4;
        }
    }
    state parse_ipv4 {
        b.extract(p.ip);
        verify(p.ip.version == 4w4, IPv4IncorrectVersion);
        verify(p.ip.ihl == 4w5, IPv4OptionsNotSupported);
        ck.clear();
        ck.update(p.ip);
        verify(ck.get() == 16w0, IPv4ChecksumError);
        transition accept;
    }
}

control Pipe(inout Parsed_packet headers, in error parseError, in InControl inCtrl, out OutControl outCtrl) {
    @name("nextHop") IPv4Address nextHop_0_0;
    @name("hasReturned_0") bool hasReturned_0_0;
    action Drop_action() {
        outCtrl.outputPort = 4w0xf;
    }
    action Set_nhop(out IPv4Address nextHop, IPv4Address ipv4_dest, PortId_t port) {
        nextHop = ipv4_dest;
        headers.ip.ttl = headers.ip.ttl + 8w255;
        outCtrl.outputPort = port;
    }
    table ipv4_match(out IPv4Address nextHop) {
        key = {
            headers.ip.dstAddr: lpm;
        }
        actions = {
            Set_nhop(nextHop);
            Drop_action;
        }
        size = 1024;
        default_action = Drop_action;
    }
    action Send_to_cpu() {
        outCtrl.outputPort = 4w0xe;
    }
    table check_ttl() {
        key = {
            headers.ip.ttl: exact;
        }
        actions = {
            Send_to_cpu;
        }
        const default_action = NoAction;
    }
    action Set_dmac(EthernetAddress dmac) {
        headers.ethernet.dstAddr = dmac;
    }
    table dmac(in IPv4Address nextHop) {
        key = {
            nextHop: exact;
        }
        actions = {
            Set_dmac;
            Drop_action;
        }
        size = 1024;
        default_action = Drop_action;
    }
    action Rewrite_smac(EthernetAddress sourceMac) {
        headers.ethernet.srcAddr = sourceMac;
    }
    table smac() {
        key = {
            outCtrl.outputPort: exact;
        }
        actions = {
            Drop_action;
            Rewrite_smac;
        }
        size = 16;
        default_action = Drop_action;
    }
    action act() {
        hasReturned_0_0 = true;
    }
    action act_0() {
        hasReturned_0_0 = false;
    }
    action act_1() {
        hasReturned_0_0 = true;
    }
    action act_2() {
        hasReturned_0_0 = true;
    }
    action act_3() {
        hasReturned_0_0 = true;
    }
    table tbl_act_0() {
        actions = {
            act_0;
        }
        const default_action = act_0();
    }
    table tbl_Drop_action() {
        actions = {
            Drop_action;
        }
        const default_action = Drop_action();
    }
    table tbl_act() {
        actions = {
            act;
        }
        const default_action = act();
    }
    table tbl_act_1() {
        actions = {
            act_1;
        }
        const default_action = act_1();
    }
    table tbl_act_2() {
        actions = {
            act_2;
        }
        const default_action = act_2();
    }
    table tbl_act_3() {
        actions = {
            act_3;
        }
        const default_action = act_3();
    }
    apply {
        tbl_act_0.apply();
        if (parseError != NoError) {
            tbl_Drop_action.apply();
            tbl_act.apply();
        }
        if (!hasReturned_0_0) {
            ipv4_match.apply(nextHop_0_0);
            if (outCtrl.outputPort == 4w0xf) 
                tbl_act_1.apply();
        }
        if (!hasReturned_0_0) {
            check_ttl.apply();
            if (outCtrl.outputPort == 4w0xe) 
                tbl_act_2.apply();
        }
        if (!hasReturned_0_0) {
            dmac.apply(nextHop_0_0);
            if (outCtrl.outputPort == 4w0xf) 
                tbl_act_3.apply();
        }
        if (!hasReturned_0_0) 
            smac.apply();
    }
}

control TopDeparser(inout Parsed_packet p, packet_out b) {
    Checksum16() ck;
    action act_4() {
        ck.clear();
        p.ip.hdrChecksum = 16w0;
        ck.update(p.ip);
        p.ip.hdrChecksum = ck.get();
    }
    table tbl_act_4() {
        actions = {
            act_4;
        }
        const default_action = act_4();
    }
    apply {
        b.emit(p.ethernet);
        if (p.ip.isValid()) {
            tbl_act_4.apply();
            b.emit(p.ip);
        }
    }
}

Simple(TopParser(), Pipe(), TopDeparser()) main;

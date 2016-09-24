#include <core.p4>

typedef bit<4> PortId;
struct InControl {
    PortId inputPort;
}

struct OutControl {
    PortId outputPort;
}

parser Parser<H>(packet_in b, out H parsedHeaders);
control Pipe<H>(inout H headers, in error parseError, in InControl inCtrl, out OutControl outCtrl);
control Deparser<H>(inout H outputHeaders, packet_out b);
package VSS<H>(Parser<H> p, Pipe<H> map, Deparser<H> d);
extern Checksum16 {
    void clear();
    void update<T>(in T data);
    bit<16> get();
}

typedef bit<48> EthernetAddress;
typedef bit<32> IPv4Address;
header Ethernet_h {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header Ipv4_h {
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
    IPv4OptionsNotSupported,
    IPv4IncorrectVersion,
    IPv4ChecksumError
}

struct Parsed_packet {
    Ethernet_h ethernet;
    Ipv4_h     ip;
}

parser TopParser(packet_in b, out Parsed_packet p) {
    @name("tmp") bool tmp_11;
    @name("tmp_0") bool tmp_12;
    @name("tmp_1") bit<16> tmp_13;
    @name("tmp_2") bool tmp_14;
    @name("tmp_3") bool tmp_15;
    @name("tmp_4") error tmp_16;
    @name("ck") Checksum16() ck;
    state start {
        b.extract<Ethernet_h>(p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x800: parse_ipv4;
        }
    }
    state parse_ipv4 {
        b.extract<Ipv4_h>(p.ip);
        tmp_11 = p.ip.version == 4w4;
        verify(tmp_11, IPv4IncorrectVersion);
        tmp_12 = p.ip.ihl == 4w5;
        verify(tmp_12, IPv4OptionsNotSupported);
        ck.clear();
        ck.update<Ipv4_h>(p.ip);
        tmp_13 = ck.get();
        tmp_14 = tmp_13 == 16w0;
        tmp_15 = tmp_14;
        tmp_16 = IPv4ChecksumError;
        verify(tmp_15, tmp_16);
        transition accept;
    }
}

control TopPipe(inout Parsed_packet headers, in error parseError, in InControl inCtrl, out OutControl outCtrl) {
    @name("nextHop") IPv4Address nextHop_1;
    @name("tmp_5") bit<8> tmp_17;
    @name("nextHop") IPv4Address nextHop_2;
    @name("nextHop") IPv4Address nextHop_3;
    @name("tmp_6") bool tmp_18;
    @name("tmp_7") bool tmp_19;
    @name("tmp_8") bool tmp_20;
    @name("tmp_9") bool tmp_21;
    @name("hasReturned") bool hasReturned_0;
    @name("nextHop") IPv4Address nextHop_0;
    @name("NoAction_1") action NoAction() {
    }
    @name("Drop_action") action Drop_action_0() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("Drop_action") action Drop_action_4() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("Drop_action") action Drop_action_5() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("Drop_action") action Drop_action_6() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("Set_nhop") action Set_nhop_0(IPv4Address ipv4_dest, PortId port) {
        nextHop_0 = ipv4_dest;
        tmp_17 = headers.ip.ttl + 8w255;
        headers.ip.ttl = tmp_17;
        outCtrl.outputPort = port;
        nextHop_2 = nextHop_0;
    }
    @name("ipv4_match") table ipv4_match() {
        key = {
            headers.ip.dstAddr: lpm;
        }
        actions = {
            Drop_action_0();
            Set_nhop_0();
        }
        size = 1024;
        default_action = Drop_action_0();
    }
    @name("Send_to_cpu") action Send_to_cpu_0() {
        outCtrl.outputPort = 4w0xe;
    }
    @name("check_ttl") table check_ttl() {
        key = {
            headers.ip.ttl: exact;
        }
        actions = {
            Send_to_cpu_0();
            NoAction();
        }
        const default_action = NoAction();
    }
    @name("Set_dmac") action Set_dmac_0(EthernetAddress dmac) {
        headers.ethernet.dstAddr = dmac;
    }
    @name("dmac") table dmac_1() {
        key = {
            nextHop_3: exact;
        }
        actions = {
            Drop_action_4();
            Set_dmac_0();
        }
        size = 1024;
        default_action = Drop_action_4();
    }
    @name("Set_smac") action Set_smac_0(EthernetAddress smac) {
        headers.ethernet.srcAddr = smac;
    }
    @name("smac") table smac_1() {
        key = {
            outCtrl.outputPort: exact;
        }
        actions = {
            Drop_action_5();
            Set_smac_0();
        }
        size = 16;
        default_action = Drop_action_5();
    }
    action act() {
        hasReturned_0 = true;
    }
    action act_0() {
        hasReturned_0 = false;
        tmp_18 = parseError != NoError;
    }
    action act_1() {
        hasReturned_0 = true;
    }
    action act_2() {
        nextHop_1 = nextHop_2;
        tmp_19 = outCtrl.outputPort == 4w0xf;
    }
    action act_3() {
        hasReturned_0 = true;
    }
    action act_4() {
        tmp_20 = outCtrl.outputPort == 4w0xe;
    }
    action act_5() {
        nextHop_3 = nextHop_1;
    }
    action act_6() {
        hasReturned_0 = true;
    }
    action act_7() {
        tmp_21 = outCtrl.outputPort == 4w0xf;
    }
    table tbl_act() {
        actions = {
            act_0();
        }
        const default_action = act_0();
    }
    table tbl_Drop_action() {
        actions = {
            Drop_action_6();
        }
        const default_action = Drop_action_6();
    }
    table tbl_act_0() {
        actions = {
            act();
        }
        const default_action = act();
    }
    table tbl_act_1() {
        actions = {
            act_2();
        }
        const default_action = act_2();
    }
    table tbl_act_2() {
        actions = {
            act_1();
        }
        const default_action = act_1();
    }
    table tbl_act_3() {
        actions = {
            act_4();
        }
        const default_action = act_4();
    }
    table tbl_act_4() {
        actions = {
            act_3();
        }
        const default_action = act_3();
    }
    table tbl_act_5() {
        actions = {
            act_5();
        }
        const default_action = act_5();
    }
    table tbl_act_6() {
        actions = {
            act_7();
        }
        const default_action = act_7();
    }
    table tbl_act_7() {
        actions = {
            act_6();
        }
        const default_action = act_6();
    }
    apply {
        tbl_act.apply();
        if (tmp_18) {
            tbl_Drop_action.apply();
            tbl_act_0.apply();
        }
        if (!hasReturned_0) {
            ipv4_match.apply();
            tbl_act_1.apply();
            if (tmp_19) 
                tbl_act_2.apply();
        }
        if (!hasReturned_0) {
            check_ttl.apply();
            tbl_act_3.apply();
            if (tmp_20) 
                tbl_act_4.apply();
        }
        if (!hasReturned_0) {
            tbl_act_5.apply();
            dmac_1.apply();
            tbl_act_6.apply();
            if (tmp_21) 
                tbl_act_7.apply();
        }
        if (!hasReturned_0) 
            smac_1.apply();
    }
}

control TopDeparser(inout Parsed_packet p, packet_out b) {
    @name("tmp_10") bit<16> tmp_22;
    @name("ck") Checksum16() ck_2;
    action act_8() {
        ck_2.clear();
        p.ip.hdrChecksum = 16w0;
        ck_2.update<Ipv4_h>(p.ip);
        tmp_22 = ck_2.get();
        p.ip.hdrChecksum = tmp_22;
    }
    table tbl_act_8() {
        actions = {
            act_8();
        }
        const default_action = act_8();
    }
    apply {
        b.emit<Ethernet_h>(p.ethernet);
        if (p.ip.isValid()) {
            tbl_act_8.apply();
        }
        b.emit<Ipv4_h>(p.ip);
    }
}

VSS<Parsed_packet>(TopParser(), TopPipe(), TopDeparser()) main;

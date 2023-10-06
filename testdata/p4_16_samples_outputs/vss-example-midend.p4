error {
    IPv4OptionsNotSupported,
    IPv4IncorrectVersion,
    IPv4ChecksumError
}
#include <core.p4>

struct InControl {
    bit<4> inputPort;
}

struct OutControl {
    bit<4> outputPort;
}

parser Parser<H>(packet_in b, out H parsedHeaders);
control Pipe<H>(inout H headers, in error parseError, in InControl inCtrl, out OutControl outCtrl);
control Deparser<H>(inout H outputHeaders, packet_out b);
package VSS<H>(Parser<H> p, Pipe<H> map, Deparser<H> d);
extern Ck16 {
    Ck16();
    void clear();
    void update<T>(in T data);
    bit<16> get();
}

header Ethernet_h {
    bit<48> dstAddr;
    bit<48> srcAddr;
    bit<16> etherType;
}

header Ipv4_h {
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

struct Parsed_packet {
    Ethernet_h ethernet;
    Ipv4_h     ip;
}

parser TopParser(packet_in b, out Parsed_packet p) {
    @name("TopParser.tmp") bit<16> tmp;
    @name("TopParser.ck") Ck16() ck_0;
    state start {
        b.extract<Ethernet_h>(p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x800: parse_ipv4;
            default: noMatch;
        }
    }
    state parse_ipv4 {
        b.extract<Ipv4_h>(p.ip);
        verify(p.ip.version == 4w4, error.IPv4IncorrectVersion);
        verify(p.ip.ihl == 4w5, error.IPv4OptionsNotSupported);
        ck_0.clear();
        ck_0.update<Ipv4_h>(p.ip);
        tmp = ck_0.get();
        verify(tmp == 16w0, error.IPv4ChecksumError);
        transition accept;
    }
    state noMatch {
        verify(false, error.NoMatch);
        transition reject;
    }
}

control TopPipe(inout Parsed_packet headers, in error parseError, in InControl inCtrl, out OutControl outCtrl) {
    @name("TopPipe.nextHop") bit<32> nextHop_0;
    @name("TopPipe.hasReturned") bool hasReturned;
    @noWarn("unused") @name(".NoAction") action NoAction_1() {
    }
    @name("TopPipe.Drop_action") action Drop_action() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("TopPipe.Drop_action") action Drop_action_1() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("TopPipe.Drop_action") action Drop_action_2() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("TopPipe.Drop_action") action Drop_action_3() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("TopPipe.Set_nhop") action Set_nhop(@name("ipv4_dest") bit<32> ipv4_dest, @name("port") bit<4> port) {
        nextHop_0 = ipv4_dest;
        headers.ip.ttl = headers.ip.ttl + 8w255;
        outCtrl.outputPort = port;
    }
    @name("TopPipe.ipv4_match") table ipv4_match_0 {
        key = {
            headers.ip.dstAddr: lpm @name("headers.ip.dstAddr");
        }
        actions = {
            Drop_action();
            Set_nhop();
        }
        size = 1024;
        default_action = Drop_action();
    }
    @name("TopPipe.Send_to_cpu") action Send_to_cpu() {
        outCtrl.outputPort = 4w0xe;
    }
    @name("TopPipe.check_ttl") table check_ttl_0 {
        key = {
            headers.ip.ttl: exact @name("headers.ip.ttl");
        }
        actions = {
            Send_to_cpu();
            NoAction_1();
        }
        const default_action = NoAction_1();
    }
    @name("TopPipe.Set_dmac") action Set_dmac(@name("dmac") bit<48> dmac_0) {
        headers.ethernet.dstAddr = dmac_0;
    }
    @name("TopPipe.dmac") table dmac_1 {
        key = {
            nextHop_0: exact @name("nextHop");
        }
        actions = {
            Drop_action_1();
            Set_dmac();
        }
        size = 1024;
        default_action = Drop_action_1();
    }
    @name("TopPipe.Set_smac") action Set_smac(@name("smac") bit<48> smac_0) {
        headers.ethernet.srcAddr = smac_0;
    }
    @name("TopPipe.smac") table smac_1 {
        key = {
            outCtrl.outputPort: exact @name("outCtrl.outputPort");
        }
        actions = {
            Drop_action_2();
            Set_smac();
        }
        size = 16;
        default_action = Drop_action_2();
    }
    @hidden action vssexample191() {
        hasReturned = true;
    }
    @hidden action act() {
        hasReturned = false;
    }
    @hidden action vssexample195() {
        hasReturned = true;
    }
    @hidden action vssexample198() {
        hasReturned = true;
    }
    @hidden action vssexample201() {
        hasReturned = true;
    }
    @hidden table tbl_act {
        actions = {
            act();
        }
        const default_action = act();
    }
    @hidden table tbl_Drop_action {
        actions = {
            Drop_action_3();
        }
        const default_action = Drop_action_3();
    }
    @hidden table tbl_vssexample191 {
        actions = {
            vssexample191();
        }
        const default_action = vssexample191();
    }
    @hidden table tbl_vssexample195 {
        actions = {
            vssexample195();
        }
        const default_action = vssexample195();
    }
    @hidden table tbl_vssexample198 {
        actions = {
            vssexample198();
        }
        const default_action = vssexample198();
    }
    @hidden table tbl_vssexample201 {
        actions = {
            vssexample201();
        }
        const default_action = vssexample201();
    }
    apply {
        tbl_act.apply();
        if (parseError != error.NoError) {
            tbl_Drop_action.apply();
            tbl_vssexample191.apply();
        }
        if (hasReturned) {
            ;
        } else {
            ipv4_match_0.apply();
            if (outCtrl.outputPort == 4w0xf) {
                tbl_vssexample195.apply();
            }
        }
        if (hasReturned) {
            ;
        } else {
            check_ttl_0.apply();
            if (outCtrl.outputPort == 4w0xe) {
                tbl_vssexample198.apply();
            }
        }
        if (hasReturned) {
            ;
        } else {
            dmac_1.apply();
            if (outCtrl.outputPort == 4w0xf) {
                tbl_vssexample201.apply();
            }
        }
        if (hasReturned) {
            ;
        } else {
            smac_1.apply();
        }
    }
}

control TopDeparser(inout Parsed_packet p, packet_out b) {
    @name("TopDeparser.ck") Ck16() ck_1;
    @hidden action vssexample213() {
        ck_1.clear();
        p.ip.hdrChecksum = 16w0;
        ck_1.update<Ipv4_h>(p.ip);
        p.ip.hdrChecksum = ck_1.get();
    }
    @hidden action vssexample211() {
        b.emit<Ethernet_h>(p.ethernet);
    }
    @hidden action vssexample218() {
        b.emit<Ipv4_h>(p.ip);
    }
    @hidden table tbl_vssexample211 {
        actions = {
            vssexample211();
        }
        const default_action = vssexample211();
    }
    @hidden table tbl_vssexample213 {
        actions = {
            vssexample213();
        }
        const default_action = vssexample213();
    }
    @hidden table tbl_vssexample218 {
        actions = {
            vssexample218();
        }
        const default_action = vssexample218();
    }
    apply {
        tbl_vssexample211.apply();
        if (p.ip.isValid()) {
            tbl_vssexample213.apply();
        }
        tbl_vssexample218.apply();
    }
}

VSS<Parsed_packet>(TopParser(), TopPipe(), TopDeparser()) main;

#include <core.p4>

error {
    IPv4OptionsNotSupported,
    IPv4IncorrectVersion,
    IPv4ChecksumError
}

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
extern Ck16 {
    Ck16();
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

struct Parsed_packet {
    Ethernet_h ethernet;
    Ipv4_h     ip;
}

parser TopParser(packet_in b, out Parsed_packet p) {
    bool tmp;
    bool tmp_0;
    bit<16> tmp_1;
    bool tmp_2;
    bool tmp_3;
    @name("ck") Ck16() ck_0;
    state start {
        b.extract<Ethernet_h>(p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x800: parse_ipv4;
        }
    }
    state parse_ipv4 {
        b.extract<Ipv4_h>(p.ip);
        tmp = p.ip.version == 4w4;
        verify(tmp, error.IPv4IncorrectVersion);
        tmp_0 = p.ip.ihl == 4w5;
        verify(tmp_0, error.IPv4OptionsNotSupported);
        ck_0.clear();
        ck_0.update<Ipv4_h>(p.ip);
        tmp_1 = ck_0.get();
        tmp_2 = tmp_1 == 16w0;
        tmp_3 = tmp_2;
        verify(tmp_3, error.IPv4ChecksumError);
        transition accept;
    }
}

control TopPipe(inout Parsed_packet headers, in error parseError, in InControl inCtrl, out OutControl outCtrl) {
    IPv4Address nextHop_0;
    bit<8> tmp_4;
    bool tmp_5;
    bool tmp_6;
    bool tmp_7;
    bool tmp_8;
    @name("Drop_action") action Drop_action_0() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("Set_nhop") action Set_nhop_0(out IPv4Address nextHop, IPv4Address ipv4_dest, PortId port) {
        nextHop = ipv4_dest;
        tmp_4 = headers.ip.ttl + 8w255;
        headers.ip.ttl = tmp_4;
        outCtrl.outputPort = port;
    }
    @name("ipv4_match") table ipv4_match_0(out IPv4Address nextHop) {
        key = {
            headers.ip.dstAddr: lpm;
        }
        actions = {
            Drop_action_0();
            Set_nhop_0(nextHop);
        }
        size = 1024;
        default_action = Drop_action_0();
    }
    @name("Send_to_cpu") action Send_to_cpu_0() {
        outCtrl.outputPort = 4w0xe;
    }
    @name("check_ttl") table check_ttl_0() {
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
    @name("dmac") table dmac_0(in IPv4Address nextHop) {
        key = {
            nextHop: exact;
        }
        actions = {
            Drop_action_0();
            Set_dmac_0();
        }
        size = 1024;
        default_action = Drop_action_0();
    }
    @name("Set_smac") action Set_smac_0(EthernetAddress smac) {
        headers.ethernet.srcAddr = smac;
    }
    @name("smac") table smac_0() {
        key = {
            outCtrl.outputPort: exact;
        }
        actions = {
            Drop_action_0();
            Set_smac_0();
        }
        size = 16;
        default_action = Drop_action_0();
    }
    apply {
        tmp_5 = parseError != error.NoError;
        if (tmp_5) {
            Drop_action_0();
            return;
        }
        ipv4_match_0.apply(nextHop_0);
        tmp_6 = outCtrl.outputPort == 4w0xf;
        if (tmp_6) 
            return;
        check_ttl_0.apply();
        tmp_7 = outCtrl.outputPort == 4w0xe;
        if (tmp_7) 
            return;
        dmac_0.apply(nextHop_0);
        tmp_8 = outCtrl.outputPort == 4w0xf;
        if (tmp_8) 
            return;
        smac_0.apply();
    }
}

control TopDeparser(inout Parsed_packet p, packet_out b) {
    bit<16> tmp_9;
    @name("ck") Ck16() ck_1;
    apply {
        b.emit<Ethernet_h>(p.ethernet);
        if (p.ip.isValid()) {
            ck_1.clear();
            p.ip.hdrChecksum = 16w0;
            ck_1.update<Ipv4_h>(p.ip);
            tmp_9 = ck_1.get();
            p.ip.hdrChecksum = tmp_9;
        }
        b.emit<Ipv4_h>(p.ip);
    }
}

VSS<Parsed_packet>(TopParser(), TopPipe(), TopDeparser()) main;

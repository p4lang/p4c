error {
    IPv4OptionsNotSupported,
    IPv4IncorrectVersion,
    IPv4ChecksumError
}
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
    bit<16> tmp;
    bool tmp_0;
    bool tmp_1;
    @name("TopParser.ck") Ck16() ck_0;
    state start {
        b.extract<Ethernet_h>(p.ethernet);
        transition select(p.ethernet.etherType) {
            16w0x800: parse_ipv4;
        }
    }
    state parse_ipv4 {
        b.extract<Ipv4_h>(p.ip);
        verify(p.ip.version == 4w4, error.IPv4IncorrectVersion);
        verify(p.ip.ihl == 4w5, error.IPv4OptionsNotSupported);
        ck_0.clear();
        ck_0.update<Ipv4_h>(p.ip);
        tmp = ck_0.get();
        tmp_0 = tmp == 16w0;
        tmp_1 = tmp_0;
        verify(tmp_1, error.IPv4ChecksumError);
        transition accept;
    }
}

control TopPipe(inout Parsed_packet headers, in error parseError, in InControl inCtrl, out OutControl outCtrl) {
    @name(".NoAction") action NoAction_0() {
    }
    IPv4Address nextHop_0;
    @name("TopPipe.Drop_action") action Drop_action() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("TopPipe.Drop_action") action Drop_action_4() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("TopPipe.Drop_action") action Drop_action_5() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("TopPipe.Drop_action") action Drop_action_6() {
        outCtrl.outputPort = 4w0xf;
    }
    @name("TopPipe.Set_nhop") action Set_nhop(IPv4Address ipv4_dest, PortId port) {
        nextHop_0 = ipv4_dest;
        headers.ip.ttl = headers.ip.ttl + 8w255;
        outCtrl.outputPort = port;
    }
    @name("TopPipe.ipv4_match") table ipv4_match_0 {
        key = {
            headers.ip.dstAddr: lpm @name("headers.ip.dstAddr") ;
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
            headers.ip.ttl: exact @name("headers.ip.ttl") ;
        }
        actions = {
            Send_to_cpu();
            NoAction_0();
        }
        const default_action = NoAction_0();
    }
    @name("TopPipe.Set_dmac") action Set_dmac(EthernetAddress dmac) {
        headers.ethernet.dstAddr = dmac;
    }
    @name("TopPipe.dmac") table dmac_0 {
        key = {
            nextHop_0: exact @name("nextHop") ;
        }
        actions = {
            Drop_action_4();
            Set_dmac();
        }
        size = 1024;
        default_action = Drop_action_4();
    }
    @name("TopPipe.Set_smac") action Set_smac(EthernetAddress smac) {
        headers.ethernet.srcAddr = smac;
    }
    @name("TopPipe.smac") table smac_0 {
        key = {
            outCtrl.outputPort: exact @name("outCtrl.outputPort") ;
        }
        actions = {
            Drop_action_5();
            Set_smac();
        }
        size = 16;
        default_action = Drop_action_5();
    }
    apply {
        bool hasReturned = false;
        if (parseError != error.NoError) {
            Drop_action_6();
            hasReturned = true;
        }
        if (!hasReturned) {
            ipv4_match_0.apply();
            if (outCtrl.outputPort == 4w0xf) {
                hasReturned = true;
            }
        }
        if (!hasReturned) {
            check_ttl_0.apply();
            if (outCtrl.outputPort == 4w0xe) {
                hasReturned = true;
            }
        }
        if (!hasReturned) {
            dmac_0.apply();
            if (outCtrl.outputPort == 4w0xf) {
                hasReturned = true;
            }
        }
        if (!hasReturned) {
            smac_0.apply();
        }
    }
}

control TopDeparser(inout Parsed_packet p, packet_out b) {
    bit<16> tmp_2;
    @name("TopDeparser.ck") Ck16() ck_1;
    apply {
        b.emit<Ethernet_h>(p.ethernet);
        if (p.ip.isValid()) {
            ck_1.clear();
            p.ip.hdrChecksum = 16w0;
            ck_1.update<Ipv4_h>(p.ip);
            tmp_2 = ck_1.get();
            p.ip.hdrChecksum = tmp_2;
        }
        b.emit<Ipv4_h>(p.ip);
    }
}

VSS<Parsed_packet>(TopParser(), TopPipe(), TopDeparser()) main;


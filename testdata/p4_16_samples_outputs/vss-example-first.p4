error {
    IPv4OptionsNotSupported,
    IPv4IncorrectVersion,
    IPv4ChecksumError
}
#include <core.p4>

typedef bit<4> PortId;
const PortId REAL_PORT_COUNT = 4w8;
struct InControl {
    PortId inputPort;
}

const PortId RECIRCULATE_IN_PORT = 4w0xd;
const PortId CPU_IN_PORT = 4w0xe;
struct OutControl {
    PortId outputPort;
}

const PortId DROP_PORT = 4w0xf;
const PortId CPU_OUT_PORT = 4w0xe;
const PortId RECIRCULATE_OUT_PORT = 4w0xd;
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
    Ck16() ck;
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
        ck.clear();
        ck.update<Ipv4_h>(p.ip);
        verify(ck.get() == 16w0, error.IPv4ChecksumError);
        transition accept;
    }
}

control TopPipe(inout Parsed_packet headers, in error parseError, in InControl inCtrl, out OutControl outCtrl) {
    action Drop_action() {
        outCtrl.outputPort = 4w0xf;
    }
    IPv4Address nextHop;
    action Set_nhop(IPv4Address ipv4_dest, PortId port) {
        nextHop = ipv4_dest;
        headers.ip.ttl = headers.ip.ttl + 8w255;
        outCtrl.outputPort = port;
    }
    table ipv4_match {
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
    action Send_to_cpu() {
        outCtrl.outputPort = 4w0xe;
    }
    table check_ttl {
        key = {
            headers.ip.ttl: exact @name("headers.ip.ttl") ;
        }
        actions = {
            Send_to_cpu();
            NoAction();
        }
        const default_action = NoAction();
    }
    action Set_dmac(EthernetAddress dmac) {
        headers.ethernet.dstAddr = dmac;
    }
    table dmac {
        key = {
            nextHop: exact @name("nextHop") ;
        }
        actions = {
            Drop_action();
            Set_dmac();
        }
        size = 1024;
        default_action = Drop_action();
    }
    action Set_smac(EthernetAddress smac) {
        headers.ethernet.srcAddr = smac;
    }
    table smac {
        key = {
            outCtrl.outputPort: exact @name("outCtrl.outputPort") ;
        }
        actions = {
            Drop_action();
            Set_smac();
        }
        size = 16;
        default_action = Drop_action();
    }
    apply {
        if (parseError != error.NoError) {
            Drop_action();
            return;
        }
        ipv4_match.apply();
        if (outCtrl.outputPort == 4w0xf) {
            return;
        }
        check_ttl.apply();
        if (outCtrl.outputPort == 4w0xe) {
            return;
        }
        dmac.apply();
        if (outCtrl.outputPort == 4w0xf) {
            return;
        }
        smac.apply();
    }
}

control TopDeparser(inout Parsed_packet p, packet_out b) {
    Ck16() ck;
    apply {
        b.emit<Ethernet_h>(p.ethernet);
        if (p.ip.isValid()) {
            ck.clear();
            p.ip.hdrChecksum = 16w0;
            ck.update<Ipv4_h>(p.ip);
            p.ip.hdrChecksum = ck.get();
        }
        b.emit<Ipv4_h>(p.ip);
    }
}

VSS<Parsed_packet>(TopParser(), TopPipe(), TopDeparser()) main;


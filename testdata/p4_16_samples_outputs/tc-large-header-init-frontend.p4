#include <core.p4>
#include <tc/pna.p4>

header ethernet_t {
    bit<48> dst;
    bit<48> src;
    bit<16> ether_type;
}

header chunk_t {
    bit<64> a;
    bit<64> b;
    bit<64> c;
    bit<64> d;
}

struct headers_t {
    ethernet_t ethernet;
    chunk_t    chunk0;
    chunk_t    chunk1;
    chunk_t    chunk2;
    chunk_t    chunk3;
    chunk_t    chunk4;
    chunk_t    chunk5;
    chunk_t    chunk6;
    chunk_t    chunk7;
    chunk_t    chunk8;
    chunk_t    chunk9;
    chunk_t    chunk10;
    chunk_t    chunk11;
    chunk_t    chunk12;
    chunk_t    chunk13;
    chunk_t    chunk14;
    chunk_t    chunk15;
    chunk_t    chunk16;
    chunk_t    chunk17;
    chunk_t    chunk18;
    chunk_t    chunk19;
    chunk_t    chunk20;
    chunk_t    chunk21;
    chunk_t    chunk22;
    chunk_t    chunk23;
    chunk_t    chunk24;
    chunk_t    chunk25;
    chunk_t    chunk26;
    chunk_t    chunk27;
    chunk_t    chunk28;
    chunk_t    chunk29;
    chunk_t    chunk30;
    chunk_t    chunk31;
    chunk_t    chunk32;
    chunk_t    chunk33;
    chunk_t    chunk34;
    chunk_t    chunk35;
    chunk_t    chunk36;
    chunk_t    chunk37;
    chunk_t    chunk38;
    chunk_t    chunk39;
    chunk_t    chunk40;
    chunk_t    chunk41;
    chunk_t    chunk42;
    chunk_t    chunk43;
    chunk_t    chunk44;
    chunk_t    chunk45;
    chunk_t    chunk46;
    chunk_t    chunk47;
    chunk_t    chunk48;
    chunk_t    chunk49;
    chunk_t    chunk50;
    chunk_t    chunk51;
    chunk_t    chunk52;
    chunk_t    chunk53;
    chunk_t    chunk54;
    chunk_t    chunk55;
    chunk_t    chunk56;
    chunk_t    chunk57;
    chunk_t    chunk58;
    chunk_t    chunk59;
    chunk_t    chunk60;
    chunk_t    chunk61;
    chunk_t    chunk62;
    chunk_t    chunk63;
}

struct metadata_t {
    bit<8> unused;
}

parser Parser(packet_in pkt, out headers_t hdr, inout metadata_t meta, in pna_main_parser_input_metadata_t istd) {
    state start {
        pkt.extract<ethernet_t>(hdr.ethernet);
        transition select(hdr.ethernet.ether_type) {
            16w0x88b5: first;
            16w0x88b6: last;
            default: accept;
        }
    }
    state first {
        pkt.extract<chunk_t>(hdr.chunk0);
        transition accept;
    }
    state last {
        pkt.extract<chunk_t>(hdr.chunk63);
        transition accept;
    }
}

control Ingress(inout headers_t hdr, inout metadata_t meta, in pna_main_input_metadata_t istd, inout pna_main_output_metadata_t ostd) {
    apply {
        if (istd.parser_error != error.NoError) {
            drop_packet();
        } else {
            send_to_port(istd.input_port);
        }
    }
}

control Deparser(packet_out pkt, inout headers_t hdr, in metadata_t meta, in pna_main_output_metadata_t ostd) {
    apply {
        pkt.emit<ethernet_t>(hdr.ethernet);
        pkt.emit<chunk_t>(hdr.chunk0);
        pkt.emit<chunk_t>(hdr.chunk63);
    }
}

PNA_NIC<headers_t, metadata_t>(Parser(), Ingress(), Deparser()) main;



#include <core.p4>
#include <psa.p4>


typedef bit<48>  EthernetAddress;

header ethernet_t {
    EthernetAddress dstAddr;
    EthernetAddress srcAddr;
    bit<16>         etherType;
}

header ipv4_t {
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

struct empty_metadata_t {
}

struct fwd_metadata_t {
}

struct metadata_t {
    fwd_metadata_t fwd_metadata;
}

struct headers_t {
    ethernet_t       ethernet;
    ipv4_t           ipv4;
}


error {
    UnhandledIPv4Options,
    BadIPv4HeaderChecksum
}



parser IngressParserImpl(packet_in buffer,
                            out headers_t parsed_hdr,
                            inout metadata_t user_meta,
                            in psa_ingress_parser_input_metadata_t istd,
                            in empty_metadata_t resubmit_meta,
                            in empty_metadata_t recirculate_meta)
{
    Checksum<bit<16>>(PSA_HashAlgorithm_t.CRC16) ck;
    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        buffer.extract(parsed_hdr.ethernet);
        transition select(parsed_hdr.ethernet.etherType) {
            0x0800: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        buffer.extract(parsed_hdr.ipv4);
	ck.clear();
	ck.update({ parsed_hdr.ipv4.version,
                parsed_hdr.ipv4.ihl,
                parsed_hdr.ipv4.diffserv,
                parsed_hdr.ipv4.totalLen,
                parsed_hdr.ipv4.identification,
                parsed_hdr.ipv4.flags,
                parsed_hdr.ipv4.fragOffset,
                parsed_hdr.ipv4.ttl,
                parsed_hdr.ipv4.protocol,
                parsed_hdr.ipv4.srcAddr,
                parsed_hdr.ipv4.dstAddr });
    verify(parsed_hdr.ipv4.hdrChecksum==ck.get(),error.BadIPv4HeaderChecksum);
    transition accept;
    }
}

control ingress(inout headers_t hdr,
                inout metadata_t user_meta,
                in    psa_ingress_input_metadata_t  istd,
                inout psa_ingress_output_metadata_t ostd)
{
    apply{
        send_to_port(ostd, istd.ingress_port);
    }
}

parser EgressParserImpl(packet_in buffer,
                        out headers_t parsed_hdr,
                        inout metadata_t user_meta,
                        in psa_egress_parser_input_metadata_t istd,
                        in empty_metadata_t normal_meta,
                        in empty_metadata_t clone_i2e_meta,
                        in empty_metadata_t clone_e2e_meta)
{
    state start {
        buffer.extract(parsed_hdr.ethernet);
        buffer.extract(parsed_hdr.ipv4);
        transition accept;
    }
}

control egress(inout headers_t hdr,
                inout metadata_t user_meta,
                in    psa_egress_input_metadata_t  istd,
                inout psa_egress_output_metadata_t ostd)
{
    apply { 
        hdr.ipv4.version=5;
    }
}



control IngressDeparserImpl(packet_out buffer,
                            out empty_metadata_t clone_i2e_meta,
                            out empty_metadata_t resubmit_meta,
                            out empty_metadata_t normal_meta,
                            inout headers_t hdr,
                            in metadata_t meta,
                            in psa_ingress_output_metadata_t istd)
{
    apply {
        buffer.emit(hdr.ethernet);
        buffer.emit(hdr.ipv4);
    }
}

control EgressDeparserImpl(packet_out buffer,
                            out empty_metadata_t clone_e2e_meta,
                            out empty_metadata_t recirculate_meta,
                            inout headers_t parsed_hdr,
                            in metadata_t meta,
                            in psa_egress_output_metadata_t istd,
                            in psa_egress_deparser_input_metadata_t edstd)
{
    Checksum<bit<16>>(PSA_HashAlgorithm_t.CRC16) ck;
    apply {
	ck.clear();
	ck.update({ parsed_hdr.ipv4.version,
                parsed_hdr.ipv4.ihl,
                parsed_hdr.ipv4.diffserv,
                parsed_hdr.ipv4.totalLen,
                parsed_hdr.ipv4.identification,
                parsed_hdr.ipv4.flags,
                parsed_hdr.ipv4.fragOffset,
                parsed_hdr.ipv4.ttl,
                parsed_hdr.ipv4.protocol,
                parsed_hdr.ipv4.srcAddr,
                parsed_hdr.ipv4.dstAddr });
	parsed_hdr.ipv4.hdrChecksum = ck.get();
    buffer.emit(parsed_hdr.ethernet);   
    buffer.emit(parsed_hdr.ipv4);
    }
}

IngressPipeline(IngressParserImpl(),
                ingress(),
                IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(),
                egress(),
                EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;
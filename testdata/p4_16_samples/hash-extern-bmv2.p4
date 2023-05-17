
#include <core.p4>
#include <bmv2/psa.p4>


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

    transition accept;
    }
}

control ingress(inout headers_t hdr,
                inout metadata_t user_meta,
                in    psa_ingress_input_metadata_t  istd,
                inout psa_ingress_output_metadata_t ostd)
{
    Hash<bit<16>>(PSA_HashAlgorithm_t.CRC16) h;
    bit<12> a=0x456;
    apply{
        send_to_port(ostd, istd.ingress_port);
        hdr.ipv4.hdrChecksum = h.get_hash({a});
    if (hdr.ipv4.hdrChecksum == 0xfe82)
        hdr.ethernet.etherType = 7;
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
    Hash<bit<16>>(PSA_HashAlgorithm_t.CRC16) h;
    bit<4> base = 0x06;
    bit<4> max = 0x0A;
    apply { 
        hdr.ipv4.hdrChecksum = h.get_hash(base, { hdr.ipv4.version,
                hdr.ipv4.ihl,
                hdr.ipv4.diffserv,
                hdr.ipv4.totalLen,
                hdr.ipv4.identification,
                hdr.ipv4.flags,
                hdr.ipv4.fragOffset,
                hdr.ipv4.ttl,
                hdr.ipv4.protocol,
                hdr.ipv4.srcAddr,
                hdr.ipv4.dstAddr }, max);
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
                            inout headers_t hdr,
                            in metadata_t meta,
                            in psa_egress_output_metadata_t istd,
                            in psa_egress_deparser_input_metadata_t edstd)
{
    apply {

    buffer.emit(hdr.ethernet);   
    buffer.emit(hdr.ipv4);
    }
}

IngressPipeline(IngressParserImpl(),
                ingress(),
                IngressDeparserImpl()) ip;

EgressPipeline(EgressParserImpl(),
                egress(),
                EgressDeparserImpl()) ep;

PSA_Switch(ip, PacketReplicationEngine(), ep, BufferingQueueingEngine()) main;

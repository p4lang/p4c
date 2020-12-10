#include <v1model.p4>
#include "../vendor.p4"

parser vendor_parser(packet_in packet,
                     out headers_t hdr,
                     inout meta_t meta,
                     inout standard_metadata_t standard_metadata)

{
    const bit<16> ETHERTYPE_IPV4 = 16w0x0800;

    state start {
        transition parse_ethernet;
    }
    state parse_ethernet {
        packet.extract(hdr.eth);
        transition select(hdr.eth.etherType) {
            ETHERTYPE_IPV4: parse_ipv4;
            default: accept;
        }
    }
    state parse_ipv4 {
        packet.extract(hdr.ipv4);
        transition accept;
    }
}

control vendor_ingress(inout headers_t hdr,
                       inout meta_t meta,
                       inout standard_metadata_t standard_meta)
{
    action a() { standard_meta.egress_spec = 0; }

    table t_exact {
  	key = {
            hdr.ipv4.dstAddr : exact;
        }
	actions = {
            a;
        }
    }

    apply {
        t_exact.apply();
    }

}

control vendor_egress(inout headers_t hdr,
                      inout meta_t meta,
                      inout standard_metadata_t standard_metadata)
{
    apply {}
}

control vendor_deparser(packet_out packet, in headers_t hdr) {
    apply {
    }
}

control verifyChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {}
}

control computeChecksum(inout headers_t hdr, inout meta_t meta) {
    apply {}
}

V1Switch(vendor_parser(),
         verifyChecksum(),
         vendor_ingress(),
         vendor_egress(),
         computeChecksum(),
         vendor_deparser()) main;


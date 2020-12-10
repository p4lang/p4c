#include <v1model.p4> // standard_metadata is defined here.

header new_header_t { bit<8> foo; }

#include "vendor_copy.p4"

struct headers_t override {
    new_header_t new_header;
}

enum Choice override {
    Third
}

enum bit<8> Rate override {
    Fast = 3
}

parser vendor_parser(packet_in packet, out headers_t hdr, inout meta_t meta,
                     inout standard_metadata_t standard_metadata) override {

  // Note by default, customer.p4 parser does not include a paser state start.
  // If a customer would like to override vendor state start, then the state
  // is added here with extends.

  state parse_ethernet override {
      packet.extract(hdr.eth);
      transition select(hdr.eth.etherType) {
          0xBEEF: parse_new_state;
          default: super.parse_ethernet.transition;
    }
  }

  state parse_new_state {
     packet.extract(hdr.new_header);
     transition super.parse_ipv4.transition;
  }

}

control Cust_ingress(inout headers_t hdr,
                       inout meta_t meta,
                       inout standard_metadata_t standard_metadata)
{
    table t_exact override {
     
    }
    apply {
        Choice c = Choice.Third;
	Rate r = Rate.Fast;
    }
}

control vendor_deparser(packet_out packet, in headers_t hdr) override {
    apply {
        packet.emit(hdr.eth);
        packet.emit(hdr.new_header);
        packet.emit(hdr.ipv4);
    }
}

V1Switch(vendor_parser(),
         verifyChecksum(),
         Cust_ingress(),
         vendor_egress(),
         computeChecksum(),
         vendor_deparser()) main override;

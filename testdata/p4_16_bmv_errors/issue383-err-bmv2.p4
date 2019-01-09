#include <v1model.p4>

struct alt_t {
  bit<1> valid;
  bit<7> port;
};

struct row_t {
  alt_t alt0;
  alt_t alt1;
};

struct local_metadata_t {
  row_t row;
};

struct mac_t
{
    bit<28>   lower28Bits;
    bit<20>   upper20Bits;
}

struct macDA_t
{
    mac_t     macDA;
}


struct parsed_packet_t {};

parser parse(packet_in pk, out parsed_packet_t h,
             inout local_metadata_t local_metadata,
             inout standard_metadata_t standard_metadata) {
  state start {
    transition accept;
  }
}

control ingress(inout parsed_packet_t h,
                inout local_metadata_t local_metadata,
	        inout standard_metadata_t standard_metadata) {
    apply {
        bit<48> mac;
	macDA_t sip;
	
        sip.macDA = (macDA_t) mac;
        mac       = (bit<48>) sip.macDA;
	
    }
}

control egress(inout parsed_packet_t hdr,
               inout local_metadata_t local_metadata,
	       inout standard_metadata_t standard_metadata) {
  apply { }
}

control deparser(packet_out b, in parsed_packet_t h) {
  apply {
   }
}

control verify_checksum(inout parsed_packet_t hdr,
                        inout local_metadata_t local_metadata) {
  apply { }
}

control compute_checksum(inout parsed_packet_t hdr,
                         inout local_metadata_t local_metadata) {
  apply { }
}

V1Switch(parse(), verify_checksum(), ingress(), egress(),
         compute_checksum(), deparser()) main;

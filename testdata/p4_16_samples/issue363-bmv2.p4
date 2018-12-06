#include <v1model.p4>

struct alt_t {
  bit<1> valid;
  bit<7> port;
};

struct row_t {
  alt_t alt0;
  alt_t alt1;
};

struct parsed_packet_t {};

struct local_metadata_t {
  row_t row;
};

struct macDA_t
{
    bit<28>   lower28Bits;
    bit<20>   upper20Bits;
}

struct sip_t
{
    bit<28>	lower28Bits;
    bit<4>	upper4Bits;
    macDA_t     macDA;
}

header our_bitvec_header {
    bit<2> g2;
    bit<3> g3;
    sip_t  sip;    
}

parser parse(packet_in pk, out parsed_packet_t hdr,
             inout local_metadata_t local_metadata,
             inout standard_metadata_t standard_metadata) {
  state start {
    transition accept;
  }
}

control ingress(inout parsed_packet_t hdr,
                inout local_metadata_t local_metadata,
	        inout standard_metadata_t standard_metadata) {
    apply {
    	sip_t sip;
       bit<48> mac;
       sip.macDA.lower28Bits = 0;
       sip.macDA.upper20Bits = 0;
	
        local_metadata.row.alt0 = local_metadata.row.alt1;
        local_metadata.row.alt0.valid = 1;
        local_metadata.row.alt1.port = local_metadata.row.alt1.port + 1;
        clone3(CloneType.I2E, 0, local_metadata.row);

        mac = sip.macDA.upper20Bits ++ sip.macDA.lower28Bits;
/*	Cast support is TODO for bmv2.
        sip.macDA = (macDA_t) mac;
        mac       = (bit<48>) sip.macDA;
*/
    }
}

control egress(inout parsed_packet_t hdr,
               inout local_metadata_t local_metadata,
	       inout standard_metadata_t standard_metadata) {
  apply { }
}

control deparser(packet_out b, in parsed_packet_t hdr) {
  apply { }
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

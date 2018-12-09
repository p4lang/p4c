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

header bitvec_hdr {
    bit<5> g2;
    bit<3> g3;
    macDA_t mac;
}

struct parsed_packet_t {
    bitvec_hdr bvh;
    bitvec_hdr bvh1;    
};

struct tst_t {
    bitvec_hdr bvh0;
}

parser parse(packet_in pk, out parsed_packet_t h,
             inout local_metadata_t local_metadata,
             inout standard_metadata_t standard_metadata) {
  state start {
    pk.extract(h.bvh);
    pk.extract(h.bvh1);
    transition accept;
  }
}

control ingress(inout parsed_packet_t h,
                inout local_metadata_t local_metadata,
	        inout standard_metadata_t standard_metadata) {
    apply {
        tst_t s;

        bit<48> mac;
        // h.bvh.mac.macDA.lower28Bits = 0;
        // h.bvh.mac.macDA.upper20Bits = 0;

        local_metadata.row.alt0 = local_metadata.row.alt1;
        local_metadata.row.alt0.valid = 1;
        local_metadata.row.alt1.port = local_metadata.row.alt1.port + 1;
        clone3(CloneType.I2E, 0, local_metadata.row);

        // mac = h.bvh.mac.macDA.upper20Bits ++ h.bvh.mac.macDA.lower28Bits;

//        Cast support is TODO for bmv2.
        h.bvh.mac.macDA = (mac_t) mac;
        mac       = (bit<48>) h.bvh.mac.macDA;

    }
}

control egress(inout parsed_packet_t hdr,
               inout local_metadata_t local_metadata,
	       inout standard_metadata_t standard_metadata) {
  apply { }
}

control deparser(packet_out b, in parsed_packet_t h) {
  apply {
     b.emit(h.bvh);
     b.emit(h.bvh1);
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

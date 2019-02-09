/*
Copyright 2019 MNK Consulting, LLC.
http://mnkcg.com

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#include <v1model.p4>

struct alt_t {
    bit<1> valid;
    bit<7> port;
};

struct row_t {
    alt_t alt0;
    alt_t alt1;
};

header bitvec_hdr {
    row_t row;
    varbit<32> v;
}

struct col_t {
    bitvec_hdr bvh;
}

struct local_metadata_t {
    row_t row0;
    row_t row1;
    col_t col;
    bitvec_hdr bvh0;
    bitvec_hdr bvh1;
    varbit<32> v;  
};

struct parsed_packet_t {
    bitvec_hdr bvh0;
    bitvec_hdr bvh1;   
};

struct tst_t {
    row_t row0;
    row_t row1;
    col_t col;  
    bitvec_hdr bvh0;
    bitvec_hdr bvh1;
}

parser parse(packet_in pk, out parsed_packet_t h,
             inout local_metadata_t local_metadata,
             inout standard_metadata_t standard_metadata) {
    bit<32> len = 0;	     
    state start {
	pk.extract(h.bvh0, 32);
	pk.extract(h.bvh1, 32);
	len = h.bvh0.sizeBits() + h.bvh1.sizeBits();
	len = h.bvh1.sizeBytes() + h.bvh1.sizeBytes();
	pk.extract(local_metadata.col.bvh, 32);
	transition accept;
    }
}

control ingress(inout parsed_packet_t h,
                inout local_metadata_t local_metadata,
                inout standard_metadata_t standard_metadata) {
    apply { }
}

control egress(inout parsed_packet_t hdr,
               inout local_metadata_t local_metadata,
               inout standard_metadata_t standard_metadata) {
    apply { }
}

control deparser(packet_out b, in parsed_packet_t h) {
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

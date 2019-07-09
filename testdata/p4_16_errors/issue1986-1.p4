#include <core.p4>
#include <v1model.p4>

header h1_t {
     bit<48> x;
 }
 header h2_t {
     bit<48> y;
 }
 struct headers {
     h1_t h1;
     h2_t h2;
 }
 struct metadata { }
 parser p(packet_in pkt,out headers hdr,inout metadata meta,
                  inout standard_metadata_t std_meta) {
     state start {
         transition parse_h1;
     }
     state parse_h1 {
         pkt.extract(hdr.h1);
         transition select(2w0) {0w0: parse_h2;
         }
     }
     state parse_h2 {
         pkt.extract(hdr.h2);
         transition accept;
     }
 }
 control c1(inout headers hdr,inout metadata meta) {
     apply { }
 }
 control c2(inout headers hdr,inout metadata meta,inout standard_metadata_t std_meta) {
     apply { }
 }
 control c3(inout headers hdr,inout metadata meta,inout standard_metadata_t std_meta) {
     apply { }
 }
control c4(inout headers hdr,inout metadata meta) {
     apply { }
 }
 control c5(packet_out pkt,in headers hdr) {
     apply { }
 }
 V1Switch(p(),c1(),c2(),c3(),c4(),c5()) main;

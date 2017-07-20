header_type egress {
  fields {
    counterrevolution : 32;
    cartographers : 8;
    wadded : 8;
    terns : 32;
    miscellaneous : 8;
  }
}
header egress heartlands;
action add_heartlands() {
  add_header(heartlands);
}

header_type headers {
  fields {
    counterrevolution : 32;
    cartographers : 8;
    wadded : 8;
    terns : 32;
    miscellaneous : 8;
  }
}
header headers heartlands_h;
action add_heartlands_h() {
  add_header(heartlands_h);
}

header_type metadata {
  fields {
    counterrevolution : 32;
    cartographers : 8;
    wadded : 8;
    terns : 32;
    miscellaneous : 8;
  }
}
header metadata heartlands_m;
action add_heartlands_m() {
  add_header(heartlands_m);
}

header_type computeChecksum {
  fields {
    counterrevolution : 32;
    cartographers : 8;
    wadded : 8;
    terns : 32;
    miscellaneous : 8;
  }
}
header computeChecksum heartlands_c;
action add_heartlands_c() {
  add_header(heartlands_c);
}

header_type verifyChecksum {
  fields {
    counterrevolution : 32;
    cartographers : 8;
    wadded : 8;
    terns : 32;
    miscellaneous : 8;
  }
}
header verifyChecksum heartlands_v;
action add_heartlands_v() {
  add_header(heartlands_v);
}

// // standard_metadata_t is predefined for v1model
// header_type standard_metadata_t {
//   fields {
//     counterrevolution : 32;
//     cartographers : 8;
//     wadded : 8;
//     terns : 32;
//     miscellaneous : 8;
//   }
// }
// header standard_metadata_t heartlands_s;
// action add_heartlands_s() {
//   add_header(heartlands_s);
// }

// // main is predefined for v1model
// header_type main {
//   fields {
//     counterrevolution : 32;
//     cartographers : 8;
//     wadded : 8;
//     terns : 32;
//     miscellaneous : 8;
//   }
// }
// header main heartlands_main;
// action add_heartlands_main() {
//   add_header(heartlands_main);
// }

parser start {
  return ingress;
}
control ingress { }

header_type headers {
  fields {
    counterrevolution : 32;
  }
}
header headers heartlands;
parser start {
  return ingress;
}
action add_heartlands() {
  add_header(heartlands);
}
control ingress { }

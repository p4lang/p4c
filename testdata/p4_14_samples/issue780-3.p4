header_type metadata {
  fields {
    counterrevolution : 32;
  }
}
header metadata heartlands;
parser start {
  return ingress;
}
action add_heartlands() {
  add_header(heartlands);
}
control ingress { }

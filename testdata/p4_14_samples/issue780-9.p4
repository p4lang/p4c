header_type exact {
  fields {
    counterrevolution : 32;
  }
}
header exact heartlands;
parser start {
  return ingress1;
}
action add_heartlands() {
  add_header(heartlands);
}
control ingress1 { }

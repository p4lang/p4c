header_type headers {
  fields {
     x : 32;
  }
}
header headers h;

parser start {
  return ingress;
}

action mask() {
    modify_field(h.x, h.x, 0x0);
}

table t {
    actions { mask; }
}

control ingress {
    apply(t);
}

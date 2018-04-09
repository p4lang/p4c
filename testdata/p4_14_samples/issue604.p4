parser start { return ingress; }
extern_type extern_test {
    attribute att_test {
        type: int;
    }
    method my_extern_method ();
}
extern extern_test my_extern_inst {
  att_test: 0xab;
}
action a() { my_extern_inst.my_extern_method(); }
table t {
    actions { a; }
}
control ingress {
    apply(t);
}
control egress {}

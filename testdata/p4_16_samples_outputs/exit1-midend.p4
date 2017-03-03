control ctrl() {
    bool hasExited;
    bit<32> a;
    bool tmp_0;
    apply {
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;

control ctrl() {
    bool hasExited;
    bit<32> a;
    apply {
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;

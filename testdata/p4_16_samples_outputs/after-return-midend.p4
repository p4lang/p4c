control ctrl() {
    bit<32> a;
    bool hasReturned_0;
    apply {
    }
}

control noop();
package p(noop _n);
p(ctrl()) main;

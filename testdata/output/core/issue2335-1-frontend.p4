control ingress(inout bit<8> a) {
    apply {
        a = 8w0;
    }
}

control i(inout bit<8> a);
package top(i _i);
top(ingress()) main;

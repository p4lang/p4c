control t(inout bit<32> b) {
    apply {
    }
}

control cs(inout bit<32> arg);
package top(cs _ctrl);
top(t()) main;

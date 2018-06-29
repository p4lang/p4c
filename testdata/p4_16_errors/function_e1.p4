bit<16> max(in bit<16> left, in bit<16> right) {
    if (left > right)
        return left;
}

control c(out bit<16> b) {
    apply {
        b = max(10, 12);
    }
}

control ctrl(out bit<16> b);
package top(ctrl _c);

top(c()) main;

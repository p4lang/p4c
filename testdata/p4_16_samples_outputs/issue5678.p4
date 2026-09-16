struct S {
    bit<8> x;
}

const S s = (S)(S){x = 1};
control c(out bit<8> result) {
    apply {
        result = s.x;
    }
}

control ctrl(out bit<8> result);
package top(ctrl _c);
top(c()) main;

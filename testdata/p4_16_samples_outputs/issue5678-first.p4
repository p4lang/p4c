struct S {
    bit<8> x;
}

const S s = (S){x = 8w1};
control c(out bit<8> result) {
    apply {
        result = 8w1;
    }
}

control ctrl(out bit<8> result);
package top(ctrl _c);
top(c()) main;

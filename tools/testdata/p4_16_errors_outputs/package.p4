control proto(out bit<32> o);
package top(in proto _c);
control c(out bit<32> o) {
    apply {
        o = 0;
    }
}

top(c(), true) main;


control proto(out bit<32> o);
package top(proto _c, bool parameter);
control c(out bit<32> o) {
    apply {
        o = 0;
    }
}

top(c(), true) main;


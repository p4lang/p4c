control c(out bit<32> x);
package top(c _c);
control my(out bit<32> x) {
    apply {
        x = 32w2;
    }
}

top(my()) main;


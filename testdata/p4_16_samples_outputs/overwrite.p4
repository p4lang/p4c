control c(out bit<32> x);
package top(c _c);
control my(out bit<32> x) {
    apply {
        x = 1;
        x = 2;
    }
}

top(my()) main;


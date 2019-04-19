control ctrl(out bit<3> _x);
package top(ctrl c);
control c_0(out bit<3> x) {
    apply {
        x = 3w1;
    }
}

top(c_0()) main;


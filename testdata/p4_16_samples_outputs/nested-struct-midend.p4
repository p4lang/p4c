struct S {
    bit<32> b;
}

struct Header {
    S      data;
    bit<1> valid;
}

struct H2 {
    Header g;
    bit<1> invalid;
}

control c(out bit<1> x) {
    @hidden action nestedstruct29() {
        x = 1w0;
    }
    @hidden table tbl_nestedstruct29 {
        actions = {
            nestedstruct29();
        }
        const default_action = nestedstruct29();
    }
    apply {
        tbl_nestedstruct29.apply();
    }
}

control ctrl(out bit<1> x);
package top(ctrl _c);
top(c()) main;

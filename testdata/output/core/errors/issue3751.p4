parser p(in bit<32> t) {
    value_set<bit<1>>(t) vs;
    state start {
        transition accept;
    }
}

parser pt(in bit<32> t);
package pp(pt t);
pp(p()) main;

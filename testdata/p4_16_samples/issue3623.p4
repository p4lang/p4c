enum bit<4> e {
    a = 1
}

control c(in bit<4> v) {
    apply {
        bool b = v == e.a;
        switch (v) {
            e.a:
            default: {}
        }
    }
}
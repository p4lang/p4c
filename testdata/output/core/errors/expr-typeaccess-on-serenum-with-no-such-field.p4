enum bit<4> e {
    e = 1
}

control c(in e v) {
    apply {
        switch (v) {
            e.a: 
            default: {
            }
        }
    }
}


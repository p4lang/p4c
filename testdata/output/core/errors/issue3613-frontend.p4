control c(in bit<4> a) {
    apply {
        switch (a) {
            4w0: {
            }
        }
    }
}

control ct(in bit<4> a);
package pt(ct t);
pt(c()) main;

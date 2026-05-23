control ingress(inout bit<8> h) {
    @name("ingress.tmp") bit<7> tmp_0;
    @name("ingress.a") action a() {
        h[0:0] = 1w0;
    }
    @hidden action issue2147l12() {
        tmp_0 = h[7:1];
    }
    @hidden action issue2147l14() {
        h[7:1] = tmp_0;
    }
    @hidden table tbl_issue2147l12 {
        actions = {
            issue2147l12();
        }
        const default_action = issue2147l12();
    }
    @hidden table tbl_a {
        actions = {
            a();
        }
        const default_action = a();
    }
    @hidden table tbl_issue2147l14 {
        actions = {
            issue2147l14();
        }
        const default_action = issue2147l14();
    }
    apply {
        tbl_issue2147l12.apply();
        tbl_a.apply();
        tbl_issue2147l14.apply();
    }
}

control c<H>(inout H h);
package top<H>(c<H> c);
top<bit<8>>(ingress()) main;

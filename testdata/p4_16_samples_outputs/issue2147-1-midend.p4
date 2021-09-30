control ingress(inout bit<8> h) {
    @name("ingress.tmp") bit<8> tmp_0;
    @name("ingress.a") action a() {
    }
    @hidden action issue21471l5() {
        tmp_0 = h;
    }
    @hidden table tbl_issue21471l5 {
        actions = {
            issue21471l5();
        }
        const default_action = issue21471l5();
    }
    @hidden table tbl_a {
        actions = {
            a();
        }
        const default_action = a();
    }
    apply {
        tbl_issue21471l5.apply();
        tbl_a.apply();
    }
}

control c<H>(inout H h);
package top<H>(c<H> c);
top<bit<8>>(ingress()) main;


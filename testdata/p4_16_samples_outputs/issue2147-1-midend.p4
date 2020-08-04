control ingress(inout bit<8> h) {
    bit<8> tmp_0;
    @name("ingress.a") action a() {
    }
    @hidden action issue21471l5() {
        tmp_0 = h;
    }
    @hidden action issue21471l7() {
        h = tmp_0;
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
    @hidden table tbl_issue21471l7 {
        actions = {
            issue21471l7();
        }
        const default_action = issue21471l7();
    }
    apply {
        tbl_issue21471l5.apply();
        tbl_a.apply();
        tbl_issue21471l7.apply();
    }
}

control c<H>(inout H h);
package top<H>(c<H> c);
top<bit<8>>(ingress()) main;


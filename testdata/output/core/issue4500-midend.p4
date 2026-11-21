enum E {
    e0
}

extern void bar(E x);
extern void baz();
control c() {
    @hidden action issue4500l10() {
        bar(E.e0);
        baz();
    }
    @hidden table tbl_issue4500l10 {
        actions = {
            issue4500l10();
        }
        const default_action = issue4500l10();
    }
    apply {
        tbl_issue4500l10.apply();
    }
}

control proto();
package top(proto p);
top(c()) main;

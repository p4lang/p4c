control ingress(inout bit<8> a) {
    @hidden action issue23351l9() {
        a = 8w0;
    }
    @hidden table tbl_issue23351l9 {
        actions = {
            issue23351l9();
        }
        const default_action = issue23351l9();
    }
    apply {
        tbl_issue23351l9.apply();
    }
}

control i(inout bit<8> a);
package top(i _i);
top(ingress()) main;

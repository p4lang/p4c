control ingress(inout bit<8> a) {
    @hidden action issue23351l3() {
        a = 8w0;
    }
    @hidden table tbl_issue23351l3 {
        actions = {
            issue23351l3();
        }
        const default_action = issue23351l3();
    }
    apply {
        tbl_issue23351l3.apply();
    }
}

control i(inout bit<8> a);
package top(i _i);
top(ingress()) main;

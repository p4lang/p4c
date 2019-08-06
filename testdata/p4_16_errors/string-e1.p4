struct S {
    string s;
}

control c() {
    apply {
        string v;
        v = "hi";
    }
}

control e();
package top(e _e);

top(c()) main;

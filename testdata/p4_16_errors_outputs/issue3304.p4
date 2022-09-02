parser p<t>(in t a);
struct s<t> {
    t field;
}

extern T gen<T>();
s<p<bit<1>>> f() {
    s<p<bit<1>>> v = gen<s<p<bit<1>>>>();
    return v;
}
control c(out s<p<bit<1>>> v) {
    apply {
        v = f();
    }
}

control _C<T>(out T t);
package top<T>(_C<T> c);
top(c()) main;


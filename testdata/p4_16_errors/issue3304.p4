parser p<t>(in t a);

struct s<t>
{
 t field;
}

extern T gen<T>();

s<p<bit>> f()
{
    s<p<bit>> v = gen<s<p<bit>>>();
    return v;
}

control c(out s<p<bit>> v) {
    apply {
        v = f();
    }
}

control _C<T>(out T t);
package top<T>(_C<T> c);

top(c()) main;

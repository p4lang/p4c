struct s<t>
{
 t field;
}

struct s1<t>
{
  t field;
}

void f()
{
    s<s1> v;
}

control c() {
    apply {
        f();
    }
}

control C();
package top(C _c);

top(c()) main;

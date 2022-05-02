extern g1{
    g1();
    void f();
}

extern g
{
    g();
    abstract void f();
    void h1();
}

extern gg
{
    gg();
    void f();
}

package myp(g1 tg, gg tgg);

bool h()
{
    myp(g1(), gg()) ty;
    g() t = {
        myp(g1(), gg()) ty;
        gg() f1;
        void f() {
            f1.f();
            this.h1();
            h();
        }
    };
    t.f();
    return true;
}

control C();
package P(C a);
control MyD() {
    @optional @name("MyD.b_0") bool b_0;
    apply {
        b_0 = !b_0;
    }
}

P(MyD()) main;


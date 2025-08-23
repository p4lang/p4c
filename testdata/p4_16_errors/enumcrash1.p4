
enum Foo { A, B, C };
extern void testfn<T>(T arg);

action test() {
    testfn(arg = Foo.D);
}

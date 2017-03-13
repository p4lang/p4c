# Testing

## Unit Testing

To run all the unit tests in P4C, one at a time, run:
```
$ make check
```

To run all the unit tests in P4C in parallel, run:
```
$ make check TESTSUITEFLAGS=-j8
```

To see a list of all the available tests, run:
```
$ make check TESTSUITEFLAGS=--list
```

To run only a subset of tests, e.g. test 123, and tests 456 through 789, run:
```
$ make check TESTSUITEFLAGS='123 456-789'
```

To run test matching a keyword, e.g. frontend, run:
```
$ make check TESTSUITEFLAGS='-k frontend'
```

To see a complete list of test options, run:
```
$ make check TESTSUITEFLAGS=--help
```

The results of a testing run are reported in `build/tests/testsuite.log`.

## Create a new test

There are two ways to create a new test case.

### Using assertions with p4ctest

You can test a class or function by making assertions about its behavior. For example, to define
a test case that makes assertations about the behaviors of IR objects. You can define a unit test
and register the test with `p4ctest` as follows.

```
static void test_eq_main(int, char*[]) {
    auto *t = IR::Type::Bits::get(16);
    IR::Constant *a = new IR::Constant(t, 10);
    IR::Constant *b = new IR::Constant(t, 10);

    assert(*a == *b);
}

P4CTEST_REGISTER("test-eq", test_eq_main);
```

All test cases for p4ctests are in `tests` directory.

### Using end-to-end test

You can test the end-to-end behavior of the compiler by compiling some sample P4 programs. For example,
to test the p4c frontend, you can provide a sample P4 program (`foo.p4`) in one of the `testdata` directories
(`p4_16_samples`).


```
// foo.p4
control c(inout bit<32> x) {
    action a(inout bit<32> b, bit<32> d) {
        b = d;

    }
    table t() {
        actions = { a(x);  }
        default_action = a(x, 0);

    }
    apply {
        t.apply();

    }

}

control proto(inout bit<32> x);
package top(proto p);

top(c()) main;
```

The autotest framework will pick up the new test automatically if it is placed in one of the existing directories.


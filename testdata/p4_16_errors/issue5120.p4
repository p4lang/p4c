@command_line("--preferSwitch")

extern void __e(in bit<28> arg);
extern void __e2(in bit<28> arg);

control C() {
	action bar(bool a, bool b) {
		bit<28> x; bit<28> y;
		switch (a) {
		b: {
				__e(x);
			}
		default: {
			if (b) {
				__e2(y);
			}
			}
		}
	}

	action baz() {
		bar(true, false);
    }

	action foo() {
		baz();
		
	}

	table t {
		actions = { foo; }
		default_action = foo;
	}

	apply {
		t.apply();
	}
}

control proto();
package top(proto p);

top(C()) main;

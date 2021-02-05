/*
Copyright 2017 VMware, Inc.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/
header hdr { bit<32> a; }

control ct(out bit<32> x, out bit<32> y);
package top(ct _ct);

bit<32> f(out bit<32> a) {
	a = 32w0;
	return a;
}
bit<32> g(in bit<32> a) {
	return a;
}

control c(out bit<32> x, out bit<32> y) {
	action simple_action() {
		y = g(x);
                f(x);
                g(x);	
	}
	
    apply {
        hdr h = { 0 };
        bool b = h.isValid();
        b = h.isValid();
        b = h.isValid();
        b = h.isValid();

        h.isValid();
        h.isValid();
        h.isValid();

	h.setValid();

        if (b) {
            x = h.a;
            x = h.a;

	    f(h.a);
	    f(h.a);

            g(h.a);
            g(h.a);
	   
        } else {
	   x = f(h.a);
           x = g(h.a);
           f(h.a);
	   
        }
	
	simple_action();
	
    }
}
top(c()) main;

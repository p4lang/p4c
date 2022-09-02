#include <core.p4>

control c();
package p(c c);

control myc () {
  action a() { }
  action b() { }
  table t {
    key = {}
    actions = { a; }
    const entries = {
      _ : a();
    }
  }
  apply {
    t.apply();
  }
}
p(myc ()) main;

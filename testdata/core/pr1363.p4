#include<core.p4>

typedef bit implementation;

extern ActionProfile {
   ActionProfile(bit<32> size); // number of distinct actions expected
}

control c() {
  table t {
    actions = { NoAction; }
    implementation = ActionProfile(32);
  }

  apply {}
}

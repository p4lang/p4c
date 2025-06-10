#include <core.p4>

control C();
package Pkg(C c);

// int<n>
extern int<8> f0();

// bit<n>
extern bit<8> f1();

// error
error { Error0, Error1 }
extern error f2();

// enum
enum e {
    e0,
    e1,
}
extern e f3();

// serializable enum
enum bit<3> s {
    s0 = 0,
    s1 = 1,
}
extern s f4();

control c() {
  action a() {
    switch (f0()) {
      (int<8>)1: {}
      (int<8>)1: {}
      default : {}
    }

    switch (f1()) {
      (bit<8>)1: {}
      (bit<8>)1: {}
      default : {}
    }

    switch (f2()) {
      error.Error1: {}
      error.Error1: {}
      default : {}
    }

    switch (f3()) {
      e.e1: {}
      e.e1: {}
      default : {}
    }

    switch (f4()) {
      s.s1: {}
      s.s1: {}
      default : {}
    }
  }

  table t {
    actions = { a; }
  }

  apply {
    t.apply();
  }
}
Pkg(c()) main;

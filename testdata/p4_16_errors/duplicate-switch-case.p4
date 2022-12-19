// https://github.com/p4lang/p4c/issues/3612
#include <core.p4>

control c(in bit<4> a)
{
  apply {
    switch (a) {
      4w0: {}
      4w1: {} // { dg-note "" }
      4w1: {} // { dg-error "" }
    }
  }
}

control ct(in bit<4> a);
package pt(ct t);
pt(c()) main;

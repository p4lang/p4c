#include <core.p4>

control c(in bit t)
{
  table tt {
    actions = {}
  }
  apply {
    if (tt.apply() == tt.apply()) {
    }
  }
}

control ct(in bit t);
package pt(ct c);

pt(c()) main;
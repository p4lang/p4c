#include <core.p4>

error { Oops }

control C() {
  apply {
    verify(8w0 == 8w1, error.Oops);
   }
}

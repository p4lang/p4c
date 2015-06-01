#ifndef _BM_RAS_H_
#define _BM_RAS_H_

#include <cassert>

#include <Judy.h>

class RandAccessUIntSet {
public:
  typedef uintptr_t mbr_t;

public:
  RandAccessUIntSet()
    : members((Pvoid_t) NULL) { }

  // returns 0 if already present (0 element added), 1 otherwise
  int add(mbr_t mbr) {
    int Rc;
    J1S(Rc, members, mbr);
    return Rc;
  }

  // returns 0 if not present (0 element removed), 1 otherwise
  int remove(mbr_t mbr) {
    int Rc;
    J1U(Rc, members, mbr);
    return Rc;
  }

  bool contains(mbr_t mbr) const {
    int Rc;
    J1T(Rc, members, mbr);
    return Rc;
  }

  size_t count() const {
    size_t nb;
    J1C(nb, members, 0, -1);
    return nb;
  }

  // n >= 0, 0 is first element in set
  mbr_t get_nth(size_t n) const {
    assert(n <= count());
    int Rc;
    mbr_t mbr;
    J1BC(Rc, members, n, mbr);
    assert(Rc == 1);
    return mbr;
  }

private:
  Pvoid_t members;
};

#endif

#ifndef _BM_PHV_H_
#define _BM_PHV_H_

#include "fields.h"

class PHV
{
  int num_fields;
  Field *fields;

public:
  PHV(int n) {
    num_fields = n;
    fields = new Field[n];
  }

  Field *get_field(int field_index) const {
    return &fields[field_index];
  }
};


#endif

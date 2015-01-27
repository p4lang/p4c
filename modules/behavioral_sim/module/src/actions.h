#ifndef _BM_ACTIONS_H_
#define _BM_ACTIONS_H_

#include "phv.h"

// will be subclassed in PD
struct ActionFn
{
  void operator()(const PHV &phv) const
  {
    // execute the action
  }
};


#endif

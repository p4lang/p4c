#ifndef _BM_ACTIONS_H_
#define _BM_ACTIONS_H_

#include "phv.h"

// will be subclassed in PD
struct ActionFn
{
  void operator()(PHV &phv) const
  {
    // execute the action
  }
};

/* if there is action data, it will actually look like this: */

/* struct myAction : ActionFn */
/* { */
/*   std::vector<Data> action_data; */
/*   myAction(const std::vector<Data> &action_data) */
/*     : action_data(action_data) {} */
/*   void operator()(PHV &phv) const */
/*   { */

/*   } */
/* }; */


#endif

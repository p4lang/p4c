#ifndef _BM_SIMPLELOG_H
#define _BM_SIMPLELOG_H_

#include <iostream>

#ifndef ENABLE_SIMPLELOG
#define ENABLE_SIMPLELOG false
#endif

#define SIMPLELOG \
  if (!ENABLE_SIMPLELOG) ; \
  else std::cout

#endif

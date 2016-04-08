/* -*-C++-*- */

#ifndef _LIB_NULL_H_
#define _LIB_NULL_H_

#include "error.h"  // for BUG macro

// Typical C contortions to transform something into a string
#define LIB_STRINGIFY(x) #x
#define LIB_TOSTRING(x) LIB_STRINGIFY(x)

#define CHECK_NULL(a) do { \
    if ((a) == nullptr) \
        BUG(__FILE__ ":" LIB_TOSTRING(__LINE__) ": Null " #a); \
} while (0)

#endif /* _LIB_NULL_H_ */

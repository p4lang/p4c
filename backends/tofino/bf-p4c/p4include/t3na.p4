#ifndef _TOFINO3_NATIVE_ARCHITECTURE_
#define _TOFINO3_NATIVE_ARCHITECTURE_

#if   __TARGET_TOFINO__ == 3    /* <CLOUDBREAK_ONLY> */
#include "tofino3_specs.p4"     /* <CLOUDBREAK_ONLY> */
#else
#error Target does not support tofino3 native architecture
#endif

#include "tofino3_arch.p4"

#endif /* _TOFINO3_NATIVE_ARCHITECTURE_ */

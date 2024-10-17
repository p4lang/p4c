#if __TARGET_TOFINO__ == 5    /* <FLATROCK_ONLY> */
#include "tofino5arch.p4"     /* <FLATROCK_ONLY> */
#else
#error Target does not support tofino5 native architecture
#endif

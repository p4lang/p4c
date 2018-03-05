#ifndef __INCLUDE_ARCH_P4__
#define __INCLUDE_ARCH_P4__

#ifdef __TARGET_BMV2__
 #ifdef __ARCH_V1MODEL__
  #include "v1model.p4"
 #endif
 #ifdef __ARCH_PSA__
  #include "psa.p4"
 #endif
#endif

#ifdef __TARGET_FRONTEND__
 #ifdef __ARCH_V1MODEL__
  #include "v1model.p4"
 #endif
 #ifdef __ARCH_PSA__
  #include "psa.p4"
 #endif
#endif

#ifdef __TARGET_GRAPH__
 #ifdef __ARCH_V1MODEL__
  #include "v1model.p4"
 #endif
 #ifdef __ARCH_PSA__
  #include "psa.p4"
 #endif
#endif

#ifdef __TARGET_EBPF__
 #ifdef __ARCH_V1MODEL__
  #include "v1model.p4"
 #endif
 #ifdef __ARCH_PSA__
  #include "psa.p4"
 #endif
#endif

#endif

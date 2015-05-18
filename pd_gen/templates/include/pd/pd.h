#ifndef _P4_PD_H_
#define _P4_PD_H_

#include "pd/pd_static.h"

#ifdef __cplusplus
extern "C" {
#endif

p4_pd_status_t ${pd_prefix}init(const char *learning_addr);

p4_pd_status_t ${pd_prefix}assign_device(int dev_id, int rpc_port_num);

#ifdef __cplusplus
}
#endif

#endif

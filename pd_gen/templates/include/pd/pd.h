#ifndef _P4_PD_H_
#define _P4_PD_H_

#include "pd/pd_static.h"

#ifdef __cplusplus
extern "C" {
#endif

p4_pd_status_t ${pd_prefix}init(p4_pd_sess_hdl_t sess_hdl,
				const char *learning_addr);

p4_pd_status_t ${pd_prefix}assign_device(p4_pd_sess_hdl_t sess_hdl,
					 int dev_id, int rpc_port_num);

p4_pd_status_t ${pd_prefix}remove_device(p4_pd_sess_hdl_t sess_hdl, int dev_id);

#ifdef __cplusplus
}
#endif

#endif

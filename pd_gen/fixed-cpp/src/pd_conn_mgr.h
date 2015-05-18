#ifndef _P4_PD_CONN_MGR_H_
#define _P4_PD_CONN_MGR_H_

#include "Runtime.h"

using namespace  ::bm_runtime;

typedef struct pd_conn_mgr_s pd_conn_mgr_t;

pd_conn_mgr_t *pd_conn_mgr_create();
void pd_conn_mgr_destroy(pd_conn_mgr_t *conn_mgr_state);

RuntimeClient *pd_conn_mgr_client(pd_conn_mgr_t *, int dev_id);

int pd_conn_mgr_client_init(pd_conn_mgr_t *, int dev_id, int thrift_port_num);

#endif

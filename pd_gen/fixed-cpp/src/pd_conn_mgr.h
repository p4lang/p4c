#ifndef _P4_PD_CONN_MGR_H_
#define _P4_PD_CONN_MGR_H_

#include "Runtime.h"

using namespace  ::bm_runtime;

void pd_conn_mgr_init();

RuntimeClient *pd_conn_mgr_client(int dev_id);

#endif

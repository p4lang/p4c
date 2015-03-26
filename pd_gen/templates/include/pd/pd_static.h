#ifndef _P4_PD_STATIC_H_
#define _P4_PD_STATIC_H_

#include <stdint.h>

typedef uint32_t p4_pd_status_t;

typedef uint32_t p4_pd_sess_hdl_t;

typedef uint32_t p4_pd_entry_hdl_t;
typedef uint32_t p4_pd_mbr_hdl_t;
typedef uint32_t p4_pd_grp_hdl_t;
/* TODO: change that, it is horrible */
typedef void* p4_pd_value_hdl_t;

#define PD_DEV_PORT_ALL 0xffff
typedef struct p4_pd_dev_target {
  uint8_t device_id; /*!< Device Identifier the API request is for */
  uint16_t dev_port_id;/*!< If specified localizes target to the resources
			 * only accessible to the port. DEV_PORT_ALL serves
			 * as a wild-card value
			 */
} p4_pd_dev_target_t;

#ifdef __cplusplus
extern "C" {
#endif

p4_pd_status_t
p4_pd_init(void);

void
p4_pd_cleanup(void);

p4_pd_status_t
p4_pd_client_init(p4_pd_sess_hdl_t *sess_hdl, uint32_t max_txn_size);

p4_pd_status_t
p4_pd_client_cleanup(p4_pd_sess_hdl_t sess_hdl);

p4_pd_status_t
p4_pd_begin_txn(p4_pd_sess_hdl_t shdl, bool isAtomic, bool isHighPri);

p4_pd_status_t
p4_pd_verify_txn(p4_pd_sess_hdl_t shdl);

p4_pd_status_t
p4_pd_abort_txn(p4_pd_sess_hdl_t shdl);

p4_pd_status_t
p4_pd_commit_txn(p4_pd_sess_hdl_t shdl, bool hwSynchronous, bool *sendRsp);

p4_pd_status_t
p4_pd_complete_operations(p4_pd_sess_hdl_t shdl);

#ifdef __cplusplus
}
#endif

#endif

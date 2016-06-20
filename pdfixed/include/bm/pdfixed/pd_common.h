/* Copyright 2013-present Barefoot Networks, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * Antonin Bas (antonin@barefootnetworks.com)
 *
 */

#ifndef BM_PDFIXED_PD_COMMON_H_
#define BM_PDFIXED_PD_COMMON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef uint32_t p4_pd_status_t;

typedef uint32_t p4_pd_sess_hdl_t;

typedef uint32_t p4_pd_entry_hdl_t;
typedef uint32_t p4_pd_mbr_hdl_t;
typedef uint32_t p4_pd_grp_hdl_t;
// TODO(unknown): change that, it is horrible
typedef void* p4_pd_value_hdl_t;

#define PD_DEV_PIPE_ALL 0xffff
typedef struct p4_pd_dev_target {
  int device_id;  // Device Identifier the API request is for
  // If specified localizes target to the resources only accessible to the
  // port. DEV_PORT_ALL serves as a wild-card value.
  uint16_t dev_pipe_id;
} p4_pd_dev_target_t;

typedef struct p4_pd_counter_value {
  uint64_t packets;
  uint64_t bytes;
} p4_pd_counter_value_t;

typedef void (*p4_pd_stat_ent_sync_cb) (int device_id, void *cookie);
typedef void (*p4_pd_stat_sync_cb) (int device_id, void *cookie);

#define COUNTER_READ_HW_SYNC (1 << 0)

// TODO(antonin): color aware meters not supported yet in bmv2
// everything is treated as color unaware
typedef enum {
  PD_METER_TYPE_COLOR_AWARE,    /* Color aware meter */
  PD_METER_TYPE_COLOR_UNAWARE,  /* Color unaware meter */
} p4_pd_meter_type_t;

typedef struct p4_pd_packets_meter_spec {
  uint32_t cir_pps;
  uint32_t cburst_pkts;
  uint32_t pir_pps;
  uint32_t pburst_pkts;
  p4_pd_meter_type_t meter_type;
} p4_pd_packets_meter_spec_t;

typedef struct p4_pd_bytes_meter_spec {
  uint32_t cir_kbps;
  uint32_t cburst_kbits;
  uint32_t pir_kbps;
  uint32_t pburst_kbits;
  p4_pd_meter_type_t meter_type;
} p4_pd_bytes_meter_spec_t;

typedef enum {ENTRY_IDLE, ENTRY_HIT} p4_pd_hit_state_t;

typedef void (*p4_pd_notify_timeout_cb) (p4_pd_entry_hdl_t entry_hdl,
                                         void *client_data);

/* mirroring definitions */
typedef enum {
  PD_MIRROR_TYPE_NORM = 0,
  PD_MIRROR_TYPE_COAL,
  PD_MIRROR_TYPE_MAX
} p4_pd_mirror_type_e;

typedef enum {
  PD_DIR_NONE = 0,
  PD_DIR_INGRESS,
  PD_DIR_EGRESS,
  PD_DIR_BOTH
} p4_pd_direction_t;

typedef uint16_t p4_pd_mirror_id_t;

#ifdef __cplusplus
}
#endif

#endif  // BM_PDFIXED_PD_COMMON_H_

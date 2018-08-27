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

namespace cpp sswitch_runtime
namespace py sswitch_runtime

struct MirroringSessionConfig {
  1:optional i32 port;
  2:optional i32 mgid;
}

enum MirroringOperationErrorCode {
  SESSION_NOT_FOUND = 1,
}

exception InvalidMirroringOperation {
  1:MirroringOperationErrorCode code;
}

service SimpleSwitch {

  // deprecated, use the mirroring_session_* RPCs instead
  i32 mirroring_mapping_add(1:i32 mirror_id, 2:i32 egress_port);
  i32 mirroring_mapping_delete(1:i32 mirror_id);
  i32 mirroring_mapping_get_egress_port(1:i32 mirror_id);

  void mirroring_session_add(1:i32 mirror_id, 2:MirroringSessionConfig config)
  throws (1:InvalidMirroringOperation ouch);
  void mirroring_session_delete(1:i32 mirror_id)
  throws (1:InvalidMirroringOperation ouch);
  MirroringSessionConfig mirroring_session_get(1:i32 mirror_id)
  throws (1:InvalidMirroringOperation ouch);

  i32 set_egress_queue_depth(1:i32 port_num, 2:i32 depth_pkts);
  i32 set_all_egress_queue_depths(1:i32 depth_pkts);
  i32 set_egress_queue_rate(1:i32 port_num, 2:i64 rate_pps);
  i32 set_all_egress_queue_rates(1:i64 rate_pps);

  // these methods are here as an experiment, prefer get_time_elapsed_us() when
  // possible
  i64 get_time_elapsed_us();
  i64 get_time_since_epoch_us();

}

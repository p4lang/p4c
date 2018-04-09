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

#include <bm/pdfixed/pd_common.h>

#include "pd_client.h"

extern int *my_devices;

extern "C" {

//:: for ra_name, ra in register_arrays.items():
//::   name = pd_prefix + "register_reset_" + ra.cname
p4_pd_status_t
${name}
(
 p4_pd_sess_hdl_t sess_hdl,
 p4_pd_dev_target_t dev_tgt
) {
  assert(my_devices[dev_tgt.device_id]);
  try {
    pd_client(dev_tgt.device_id).c->bm_register_reset(0, "${ra_name}");
  } catch (InvalidRegisterOperation &iro) {
    const char *what =
      _RegisterOperationErrorCode_VALUES_TO_NAMES.find(iro.code)->second;
    std::cout << "Invalid register (" << "${ra_name}" << ") operation ("
	      << iro.code << "): " << what << std::endl;
    return iro.code;
  }
  return 0;
}

//:: #endfor

}

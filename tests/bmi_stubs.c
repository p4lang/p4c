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

#include <stdbool.h>

typedef void bmi_port_mgr_t;

int bmi_start_mgr(bmi_port_mgr_t* port_mgr) {
  (void) port_mgr;
  return 0;
}

int bmi_port_create_mgr(bmi_port_mgr_t **port_mgr) {
  (void) port_mgr;
  return 0;
}

int bmi_set_packet_handler(bmi_port_mgr_t *port_mgr) {
  (void) port_mgr;
  return 0;
}

int bmi_port_send(bmi_port_mgr_t *port_mgr) {
  (void) port_mgr;
  return 0;
}

int bmi_port_interface_add(bmi_port_mgr_t *port_mgr) {
  (void) port_mgr;
  return 0;
}

int bmi_port_interface_remove(bmi_port_mgr_t *port_mgr, int port_num) {
  (void) port_mgr;
  return 0;
}

int bmi_port_destroy_mgr(bmi_port_mgr_t *port_mgr) {
  (void) port_mgr;
  return 0;
}

int bmi_port_interface_is_up(bmi_port_mgr_t* port_mgr, int port_num, bool *is_up) {
  (void) port_mgr;
  return 0;
}

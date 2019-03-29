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

#include "BMI/bmi_port.h"

// BMI stubs to avoid having to link with BMI and its deps for unit tests.

#define UNUSED(x) (void)(x)

int bmi_start_mgr(bmi_port_mgr_t *port_mgr) {
  UNUSED(port_mgr);
  return 0;
}

int bmi_port_create_mgr(bmi_port_mgr_t **port_mgr, int max_port_count) {
  UNUSED(port_mgr);
  return 0;
}

int bmi_set_packet_handler(bmi_port_mgr_t *port_mgr,
                           bmi_packet_handler_t packet_handler,
                           void *cookie) {
  UNUSED(port_mgr); UNUSED(packet_handler); UNUSED(cookie);
  return 0;
}

int bmi_port_send(bmi_port_mgr_t *port_mgr, int port_num,
                  const char *buffer, int len) {
  UNUSED(port_mgr); UNUSED(port_num); UNUSED(buffer); UNUSED(len);
  return 0;
}

int bmi_port_interface_add(bmi_port_mgr_t *port_mgr,
                           const char *ifname, int port_num,
                           const char *pcap_input_dump,
                           const char *pcap_output_dump) {
  UNUSED(port_mgr); UNUSED(ifname); UNUSED(port_num);
  UNUSED(pcap_input_dump); UNUSED(pcap_output_dump);
  return 0;
}

int bmi_port_interface_remove(bmi_port_mgr_t *port_mgr, int port_num) {

  UNUSED(port_mgr); UNUSED(port_num);
  return 0;
}

int bmi_port_destroy_mgr(bmi_port_mgr_t *port_mgr) {
  UNUSED(port_mgr);
  return 0;
}

int bmi_port_interface_is_up(bmi_port_mgr_t *port_mgr,
                             int port_num, bool *is_up) {
  UNUSED(port_mgr); UNUSED(port_num); UNUSED(is_up);
  return 0;
}

int bmi_port_get_stats(bmi_port_mgr_t *port_mgr,
                       int port_num, bmi_port_stats_t *port_stats) {
  UNUSED(port_mgr); UNUSED(port_num); UNUSED(port_stats);
  return 0;
}

int bmi_port_clear_stats(bmi_port_mgr_t *port_mgr,
                         int port_num, bmi_port_stats_t *port_stats) {
  UNUSED(port_mgr); UNUSED(port_num); UNUSED(port_stats);
  return 0;
}

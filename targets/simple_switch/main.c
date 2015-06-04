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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <inttypes.h>

#include <unistd.h>

#include "BMI/bmi_port.h"

#include "simple_switch.h"

#define CHECK(x) assert(!x)

// #define PCAP_DUMP

static bmi_port_mgr_t *port_mgr;

static void packet_handler(int port_num, const char *buffer, int len) {
  printf("Received packet of length %d on port %d\n", len, port_num);
  packet_accept(port_num, buffer, len);
}

static void transmit_fn(int port_num, const char *buffer, int len) {
  bmi_port_send(port_mgr, port_num, buffer, len);
}

int 
main(int argc, char* argv[])
{
    (void)(argc);  /* COMPILER_REFERENCE */
    (void)(argv);

    printf("Initializing BMI port manager\n");

    CHECK(bmi_port_create_mgr(&port_mgr));

    printf("Adding all 4 ports\n");

#ifdef PCAP_DUMP
    CHECK(bmi_port_interface_add(port_mgr, "veth1", 1, "port1.pcap"));
    CHECK(bmi_port_interface_add(port_mgr, "veth3", 2, "port2.pcap"));
    CHECK(bmi_port_interface_add(port_mgr, "veth5", 3, "port3.pcap"));
    CHECK(bmi_port_interface_add(port_mgr, "veth7", 4, "port4.pcap"));
#else
    CHECK(bmi_port_interface_add(port_mgr, "veth1", 1, NULL));
    CHECK(bmi_port_interface_add(port_mgr, "veth3", 2, NULL));
    CHECK(bmi_port_interface_add(port_mgr, "veth5", 3, NULL));
    CHECK(bmi_port_interface_add(port_mgr, "veth7", 4, NULL));
#endif

    start_processing(transmit_fn);

    CHECK(bmi_set_packet_handler(port_mgr, packet_handler));

    while(1) {
      sleep(5);
    }

    bmi_port_destroy_mgr(port_mgr);

    /* printf("testmod target: no-op\n"); */
    
    return 0; 
}


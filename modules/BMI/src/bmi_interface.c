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

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <pcap/pcap.h>
#include "bmi_interface.h"

typedef struct bmi_interface_s {
  pcap_t *pcap;
  int fd;
  pcap_dumper_t *pcap_input_dumper;
  pcap_dumper_t *pcap_output_dumper;
} bmi_interface_t;

/* static get_version(int *x, int *y, int *z) { */
/*   const char *str = pcap_lib_version(); */
/*   sscanf(str, "%*s %*s %d.%d.%d", x, y, z); */
/* } */

int bmi_interface_create(bmi_interface_t **bmi, const char *device) {
  bmi_interface_t *bmi_ = malloc(sizeof(bmi_interface_t));

  if(!bmi_) return -1;

  bmi_->pcap_input_dumper = NULL;
  bmi_->pcap_output_dumper = NULL;

  char errbuf[PCAP_ERRBUF_SIZE];
  bmi_->pcap = pcap_create(device, errbuf);

  if(!bmi_->pcap) {
    free(bmi_);
    return -1;
  }

  if(pcap_set_promisc(bmi_->pcap, 1) != 0) {
    pcap_close(bmi_->pcap);
    free(bmi_);
    return -1;
  }

#ifdef WITH_PCAP_FIX
  if(pcap_set_timeout(bmi_->pcap, 1) != 0) {
    pcap_close(bmi_->pcap);
    free(bmi_);
    return -1;
  }

  if(pcap_set_immediate_mode(bmi_->pcap, 1) != 0) {
    pcap_close(bmi_->pcap);
    free(bmi_);
    return -1;
  }
#endif

  if (pcap_activate(bmi_->pcap) != 0) {
    pcap_close(bmi_->pcap);
    free(bmi_);
    return -1;
  }

  bmi_->fd = pcap_get_selectable_fd(bmi_->pcap);
  if(bmi_->fd < 0) {
    pcap_close(bmi_->pcap);
    free(bmi_);
    return -1;
  }

  *bmi = bmi_;
  return 0;
}

int bmi_interface_destroy(bmi_interface_t *bmi) {
  pcap_close(bmi->pcap);
  if(bmi->pcap_input_dumper) pcap_dump_close(bmi->pcap_input_dumper);
  if(bmi->pcap_output_dumper) pcap_dump_close(bmi->pcap_output_dumper);
  free(bmi);
  return 0;
}

int bmi_interface_add_dumper(bmi_interface_t *bmi, const char *filename, bmi_dumper_kind_t dumper_kind) {
  pcap_dumper_t* dumper = pcap_dump_open(bmi->pcap, filename);
  if (dumper == NULL)
    return -1;
  switch (dumper_kind)
  {
  case bmi_input_dumper:
    bmi->pcap_input_dumper = dumper;
    break;
  case bmi_output_dumper:
    bmi->pcap_output_dumper = dumper;
    break;
  default:
    return -1;
  }
  return 0;
}

int bmi_interface_send(bmi_interface_t *bmi, const char *data, int len) {
  if(bmi->pcap_output_dumper) {
    struct pcap_pkthdr pkt_header;
    memset(&pkt_header, 0, sizeof(pkt_header));
    gettimeofday(&pkt_header.ts, NULL);
    pkt_header.caplen = len;
    pkt_header.len = len;
    pcap_dump((unsigned char *) bmi->pcap_output_dumper, &pkt_header,
	      (unsigned char *) data);
    pcap_dump_flush(bmi->pcap_output_dumper);
  }
  return pcap_sendpacket(bmi->pcap, (unsigned char *) data, len);
}

/* Does not make a copy! */
int bmi_interface_recv(bmi_interface_t *bmi, const char **data) {
  struct pcap_pkthdr *pkt_header;
  const unsigned char *pkt_data;

  if(pcap_next_ex(bmi->pcap, &pkt_header, &pkt_data) != 1) {
    return -1;
  }

  if(pkt_header->caplen != pkt_header->len) {
    return -1;
  }

  if(bmi->pcap_input_dumper) {
    pcap_dump((unsigned char *) bmi->pcap_input_dumper, pkt_header, pkt_data);
    pcap_dump_flush(bmi->pcap_input_dumper);
  }

  *data = (const char *) pkt_data;

  return pkt_header->len;
}

int bmi_interface_recv_with_copy(bmi_interface_t *bmi, char *data, int max_len) {
  int rv;
  struct pcap_pkthdr *pkt_header;
  const unsigned char *pkt_data;

  if(pcap_next_ex(bmi->pcap, &pkt_header, &pkt_data) != 1) {
    return -1;
  }

  if(pkt_header->caplen != pkt_header->len) {
    return -1;
  }

  if(bmi->pcap_input_dumper) {
    pcap_dump((unsigned char *) bmi->pcap_input_dumper, pkt_header, pkt_data);
    pcap_dump_flush(bmi->pcap_input_dumper);
  }

  rv = (max_len < pkt_header->len) ? max_len : pkt_header->len;

  memcpy(data, pkt_data, rv);

  return rv;
}

int bmi_interface_get_fd(bmi_interface_t *bmi) {
  return bmi->fd;
}

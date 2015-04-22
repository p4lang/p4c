#include <iostream>

#include <nanomsg/pubsub.h>

#include <cassert>

#include <netinet/in.h>

#include "nn.h"

typedef struct {
  int switch_id;
  int list_id;
  unsigned long long buffer_id;
  unsigned int num_samples;
} __attribute__((packed)) learn_hdr_t;

typedef struct {
  char src_addr[6];
  short ingress_port;
} __attribute__((packed)) sample_t;

int main() {
  nn::socket s(AF_SP, NN_SUB);
  s.setsockopt(NN_SUB, NN_SUB_SUBSCRIBE, "", 0);
  s.connect("ipc:///tmp/test_bm_learning.ipc");

  struct nn_msghdr msghdr;
  struct nn_iovec iov[2];
  learn_hdr_t learn_hdr;
  char data[2048];
  iov[0].iov_base = &learn_hdr;
  iov[0].iov_len = sizeof(learn_hdr);
  iov[1].iov_base = data;
  iov[1].iov_len = sizeof(data); // apparently only max size needed ?
  memset(&msghdr, 0, sizeof(msghdr));
  msghdr.msg_iov = iov;
  msghdr.msg_iovlen = 2;

  while(s.recvmsg(&msghdr, 0) >= 0) {
    std::cout << "I received " << learn_hdr.num_samples << " samples" << std::endl;

    for(unsigned int i = 0; i < learn_hdr.num_samples; i++) {
      sample_t *sample = ((sample_t *) data) + i;

      sample->ingress_port = ntohs(sample->ingress_port);

      std::cout << "ingress port is " << sample->ingress_port << std::endl;
    }
  }

  return 0;
}

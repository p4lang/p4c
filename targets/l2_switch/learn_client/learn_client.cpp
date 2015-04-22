#include <iostream>

#include <nanomsg/pubsub.h>

#include <cassert>

#include <netinet/in.h>

#include "nn.h"

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "Runtime.h"

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

using namespace bm_runtime;

using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

int main() {

  boost::shared_ptr<TTransport> socket(new TSocket("localhost", 9090));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

  RuntimeClient client(protocol);

  transport->open();

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
    std::cout << "I received " << learn_hdr.num_samples << " samples"
	      << std::endl;

    for(unsigned int i = 0; i < learn_hdr.num_samples; i++) {
      sample_t *sample = ((sample_t *) data) + i;

      std::cout << "ingress port is " << ntohs(sample->ingress_port)
		<< std::endl;

      std::vector<std::string> match_key =
	{std::string(sample->src_addr, sizeof(sample->src_addr))};

      client.bm_table_add_exact_match_entry("smac", "_nop",
					    match_key,
					    std::vector<std::string>());

      std::vector<std::string> action_data =
	{std::string((char *) &sample->ingress_port, 2)};

      client.bm_table_add_exact_match_entry("dmac", "forward",
					    match_key,
					    action_data);
    }

    client.bm_learning_ack_buffer(learn_hdr.list_id, learn_hdr.buffer_id);
  }

  return 0;
}

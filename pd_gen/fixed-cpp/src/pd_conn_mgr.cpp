#include <iostream>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "pd_conn_mgr.h"

#define THRIFT_BASE_PORT 9090

#define NUM_DEVICES 256

static RuntimeClient *clients[NUM_DEVICES];

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

void pd_conn_mgr_init() {
  // TODO: only support one device now
  for(int dev_id = 0; dev_id < 1; dev_id++) {
    int port_num = THRIFT_BASE_PORT + dev_id;
    boost::shared_ptr<TTransport> socket(new TSocket("localhost", port_num));
    boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
    boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
    clients[dev_id] = new RuntimeClient(protocol);

    try {
      transport->open();
    }
    catch (TException& tx) {
      std::cout << "Could not connect to port " << port_num
		<< "(device " << dev_id << ")" << std::endl;
    }
  }
}

RuntimeClient *pd_conn_mgr_client(int dev_id) {
  return clients[dev_id];
}

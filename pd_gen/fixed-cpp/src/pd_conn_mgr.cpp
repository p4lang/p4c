#include <iostream>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "pd_conn_mgr.h"

#define NUM_DEVICES 256

struct pd_conn_mgr_s {
  RuntimeClient *clients[NUM_DEVICES];
};

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

pd_conn_mgr_t *pd_conn_mgr_create() {
  pd_conn_mgr_t *conn_mgr_state = (pd_conn_mgr_t *) malloc(sizeof(pd_conn_mgr_t));
  memset(conn_mgr_state, 0, sizeof(pd_conn_mgr_t));
  return conn_mgr_state;
}

void pd_conn_mgr_destroy(pd_conn_mgr_t *conn_mgr_state) {
  // close connections?
  free(conn_mgr_state);
}

int pd_conn_mgr_client_init(pd_conn_mgr_t *conn_mgr_state,
			    int dev_id, int thrift_port_num) {
  assert(!conn_mgr_state->clients[dev_id]);

  boost::shared_ptr<TTransport> socket(new TSocket("localhost", thrift_port_num));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

  conn_mgr_state->clients[dev_id] = new RuntimeClient(protocol);

  try {
    transport->open();
  }
  catch (TException& tx) {
    std::cout << "Could not connect to port " << thrift_port_num
	      << "(device " << dev_id << ")" << std::endl;

    return 1;
  }

  return 0;
}

RuntimeClient *pd_conn_mgr_client(pd_conn_mgr_t *conn_mgr_state,
				  int dev_id) {
  return conn_mgr_state->clients[dev_id];
}

#include <iostream>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>

#include "pd_conn_mgr.h"

#define NUM_DEVICES 256

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

struct pd_conn_mgr_s {
  RuntimeClient *clients[NUM_DEVICES];
  boost::shared_ptr<TTransport> transports[NUM_DEVICES];
};

pd_conn_mgr_t *pd_conn_mgr_create() {
  pd_conn_mgr_t *conn_mgr_state = (pd_conn_mgr_t *) malloc(sizeof(pd_conn_mgr_t));
  memset(conn_mgr_state->clients, 0, sizeof(conn_mgr_state->clients));
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

  conn_mgr_state->transports[dev_id] = transport;
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

int pd_conn_mgr_client_close(pd_conn_mgr_t *conn_mgr_state, int dev_id) {
  assert(conn_mgr_state->clients[dev_id]);
  conn_mgr_state->transports[dev_id]->close();
  delete conn_mgr_state->clients[dev_id];
  conn_mgr_state->clients[dev_id] = NULL;
  return 0;
}

RuntimeClient *pd_conn_mgr_client(pd_conn_mgr_t *conn_mgr_state,
				  int dev_id) {
  return conn_mgr_state->clients[dev_id];
}

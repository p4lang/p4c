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

#include <iostream>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include <thrift/protocol/TMultiplexedProtocol.h>

#include "pd_conn_mgr.h"

#define NUM_DEVICES 256

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;

struct client_t {
  StandardClient *client;
  McClient *mc_client;
};

struct pd_conn_mgr_s {
  client_t clients[NUM_DEVICES];
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
  assert(!conn_mgr_state->clients[dev_id].client);

  boost::shared_ptr<TTransport> socket(new TSocket("localhost", thrift_port_num));
  boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
  boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));

  boost::shared_ptr<TMultiplexedProtocol> standard_protocol(
    new TMultiplexedProtocol(protocol, "standard")
  );
  boost::shared_ptr<TMultiplexedProtocol> mc_protocol(
    new TMultiplexedProtocol(protocol, "simple_pre_lag")
  );

  conn_mgr_state->transports[dev_id] = transport;
  conn_mgr_state->clients[dev_id].client = new StandardClient(standard_protocol);
  conn_mgr_state->clients[dev_id].mc_client = new McClient(mc_protocol);

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
  assert(conn_mgr_state->clients[dev_id].client);
  conn_mgr_state->transports[dev_id]->close();
  delete conn_mgr_state->clients[dev_id].client;
  delete conn_mgr_state->clients[dev_id].mc_client;
  conn_mgr_state->clients[dev_id].client = NULL;
  conn_mgr_state->clients[dev_id].mc_client = NULL;
  return 0;
}

Client *pd_conn_mgr_client(pd_conn_mgr_t *conn_mgr_state,
			   int dev_id) {
  return conn_mgr_state->clients[dev_id].client;
}

McClient *pd_conn_mgr_mc_client(pd_conn_mgr_t *conn_mgr_state,
				int dev_id) {
  return conn_mgr_state->clients[dev_id].mc_client;
}

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

#include <thread>

#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/server/TThreadedServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/processor/TMultiplexedProcessor.h>

#include "Standard_server.ipp"
#include "SimplePreLAG_server.ipp"

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;

using boost::shared_ptr;

using ::bm_runtime::standard::StandardHandler;
using ::bm_runtime::standard::StandardProcessor;
using ::bm_runtime::simple_pre_lag::SimplePreLAGHandler;
using ::bm_runtime::simple_pre_lag::SimplePreLAGProcessor;

void run_server(int port) {
  shared_ptr<StandardHandler> standard_handler(new StandardHandler());
  shared_ptr<SimplePreLAGHandler> simple_pre_lag_handler(new SimplePreLAGHandler());

  shared_ptr<TMultiplexedProcessor> processor(new TMultiplexedProcessor());
  processor->registerProcessor(
    "standard",
    shared_ptr<TProcessor>(new StandardProcessor(standard_handler))
  );
  processor->registerProcessor(
    "simple_pre_lag",
    shared_ptr<TProcessor>(new SimplePreLAGProcessor(simple_pre_lag_handler))
  );

  shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
  shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
  shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

  TThreadedServer server(processor, serverTransport, transportFactory, protocolFactory);
  server.serve();
}

int start_server() {
  int port = 9090;
  std::thread t(run_server, port);
  t.detach();
  return 0;
}
